cmake_minimum_required(VERSION 3.18)
#project(rpcf_python LANGUAGES C CXX)

# Include the standard module that provides CMAKE_INSTALL_FULL_LIBDIR
include(GNUInstallDirs)

# ==============================================================================
# 1. Map Variables and Extension Suffixes (Hooks into your Master CMake)
# ==============================================================================
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Emulates the shell importlib machinery execution via native CMake properties
execute_process(
    COMMAND python3 -c "import importlib.machinery; print(importlib.machinery.EXTENSION_SUFFIXES[0], end='')"
    OUTPUT_VARIABLE Python3_EXTENSION_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PYTHON_EXT1 "rpcbase${Python3_EXTENSION_SUFFIX}")
set(PYTHON_EXT "${PYTHON_EXT1}.${VERSION}") # Assumes ${VERSION} comes from your master file

# ==============================================================================
# 2. Replicate Environment OS Profiling (genpipopt & IS_VENV targets)
# ==============================================================================
set(pipopt "")
if(EXISTS "/etc/os-release")
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    
    # Extract ID
    string(REGEX MATCH "ID=([^\n]+)" OS_ID_MATCH "${OS_RELEASE_CONTENT}")
    string(REPLACE "ID=" "" OS_NAME "${CMAKE_MATCH_1}")
    string(REPLACE "\"" "" OS_NAME "${OS_NAME}")
    
    # Extract VERSION_ID
    string(REGEX MATCH "VERSION_ID=([^\n]+)" OS_VER_MATCH "${OS_RELEASE_CONTENT}")
    string(REPLACE "VERSION_ID=" "" OS_VERSION "${CMAKE_MATCH_1}")
    string(REPLACE "\"" "" OS_VERSION "${OS_VERSION}")

    if((OS_NAME STREQUAL "debian" OR OS_NAME STREQUAL "raspbian") AND OS_VERSION VERSION_GREATER_EQUAL "12")
        set(pipopt "--break-system-packages")
    elseif(OS_NAME STREQUAL "ubuntu" AND OS_VERSION VERSION_GREATER_EQUAL "23")
        set(pipopt "--break-system-packages")
    endif()
endif()

if(pipopt)
    message(STATUS "Warning: break-system-packages enabled")
endif()

# Determine venv configuration mapping
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import sys; print('true' if sys.prefix != sys.base_prefix else 'false')"
    OUTPUT_VARIABLE IS_VENV OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(IS_VENV STREQUAL "false")
    set(useropt "--user")
else()
    set(useropt "")
endif()

# ==============================================================================
# 3. Target "all" Execution Sequences
# ==============================================================================
if(ENABLE_FUSE3)
    set(LINK_FUSE "1")
else()
    set(LINK_FUSE "0")
endif()

set(TARGET_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml.tmpl")

if(EXISTS "${TARGET_CONF_FILE}")
    # 1. Read the raw file content
    file(READ "${TARGET_CONF_FILE}" FILE_CONTENTS)

    # 3. Perform a regular expression replacement on the line
    # Matches 'library-dirs = [' followed by any characters up to the matching ']'
    string(REGEX REPLACE
        "library-dirs[ \t]*=[ \t]*[^\r\n]*[\r\n]"
        "library-dirs = [\"${CMAKE_BINARY_DIR}/combase\", \"${CMAKE_BINARY_DIR}/ipc\",\"${CMAKE_BINARY_DIR}/fuse\"]\n"
        MODIFIED_CONTENTS
        "${FILE_CONTENTS}"
    )

    # 4. Write the clean, updated string back to the file
    file(WRITE "${TARGET_CONF_FILE}.1" "${MODIFIED_CONTENTS}")
    message(STATUS "Successfully updated pyproject.toml.tmpl.")
else()
    message(FATAL_ERROR "Configuration file missing: ${TARGET_CONF_FILE}")
endif()


# Match your SIP version runtime conditional pipelines
if(SIPBUILD OR SIPVER EQUAL 5)
    set(BUILD_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E env FUSE3=${LINK_FUSE} bash gentoml.sh "${CMAKE_INSTALL_FULL_LIBDIR}" "${TARGET_CONF_FILE}.1"
        COMMAND bash -lc "CXXFLAGS= ${Python3_EXECUTABLE} -m sipbuild.tools.build --verbose --build-dir ./sipbuild"
        VERBATIM
        COMMAND bash -lc "sofile=`find ./sipbuild -name ${PYTHON_EXT1}`\;if [ ! -z \"\${sofile}\" ]\; then rm -f rpcf/rpcbase*.so*\;cp \${sofile} rpcf\;\
cd rpcf\;ln -s ${PYTHON_EXT1} ${PYTHON_EXT}\;fi"
        VERBATIM
    )
elseif(SIP4 OR SIPVER EQUAL 4)
    set(ARMBUILD "0")
    if(COMMON_LINK MATCHES "latomic")
        set(ARMBUILD "1")
    endif()
    
    set(DBGBUILD "0")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DBGBUILD "1")
    endif()

    set(BUILD_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E env 
            FUSE3=${LINK_FUSE}
            CC=${CMAKE_C_COMPILER}
            CXX=${CMAKE_CXX_COMPILER}
            PKG_CONFIG=${PKG_CONFIG}
            PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR}
            PKG_CONFIG_PATH=${PKG_CONFIG_PATH}
            PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR}
            SYSROOT=${with_sysroot}
            DBGBUILD=${DBGBUILD}
            ARMBUILD=${ARMBUILD}
            ${Python3_EXECUTABLE} build-4.9.py ${CMAKE_INSTALL_FULL_LIBDIR}
        COMMAND bash -lc "rm -f rpcf/rpcbase*.so*\;cp ./sip4build/rpcbase.so rpcf/${PYTHON_EXT1}\;\
cd rpcf\;ln -s ${PYTHON_EXT1} ${PYTHON_EXT}\;fi"
    )
endif()

add_custom_target(rpcf_all ALL ${BUILD_COMMANDS} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(rpcf_all ipc combase)
if( ENABLE_FUSE3 )
add_dependencies(rpcf_all fuseif)
endif()

# Execute the python script and capture the stdout
# 1. Check if running inside a Python virtual environment (venv)
execute_process(
    COMMAND python3 -c "import sys; print('true' if sys.prefix != sys.base_prefix else 'false')"
    OUTPUT_VARIABLE IS_VENV
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 2. Define the new arguments based on the venv status
if("${IS_VENV}" STREQUAL "false")
    # Replacement for 'setup.py bdist_wheel' when NO venv is active
    set(WHEEL_BUILD_ARGS -m build ${CMAKE_CURRENT_BINARY_DIR} --wheel --no-isolation --outdir dist/)
else()
    # Replacement for 'setup.py bdist_wheel' when inside a venv
    set(WHEEL_BUILD_ARGS -m pip wheel ${CMAKE_CURRENT_BINARY_DIR} --no-deps --no-build-isolation -w dist/)
endif()

# ==============================================================================
# 4. Installation Script Triggers
# ==============================================================================
set(CMAKE_VERBOSE_MAKEFILE ON)
install(CODE "
    # Staging Context Check (Emulating DESTDIR verification structures)

    if(NOT \"\$ENV{DESTDIR}\" STREQUAL \"\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy rpcf/${PYTHON_EXT} ./${PYTHON_EXT}.bak
            WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")

        set(symdir \"${CMAKE_CURRENT_SOURCE_DIR}/../bldpkg/symlibs/dbgsyms\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory \"\${symdir}\")

        execute_process(COMMAND bash ./stripsym.sh rpcf/${PYTHON_EXT} \"\${symdir}\"
            WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
    endif()

    # User Privilege Management Blocks 
    if(NOT \"\$ENV{SUDO_USER}\" STREQUAL \"\")
        set(RUN_PREFIX su -c)
        set(RUN_SUFFIX \"\$ENV{SUDO_USER}\")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} -E
        rm -rf \"${CMAKE_CURRENT_BINARY_DIR}/rpcf\")

    execute_process(COMMAND ${CMAKE_COMMAND} -E
        create_symlink \"${CMAKE_CURRENT_SOURCE_DIR}/rpcf\" \"${CMAKE_CURRENT_BINARY_DIR}/rpcf\")

    execute_process(
        COMMAND \${RUN_PREFIX} \"${Python3_EXECUTABLE}\" ${WHEEL_BUILD_ARGS} \${RUN_SUFFIX}
        WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
        )

    # Dynamic lookup resolution matching 'ls -1t dist | head -n 1'
    file(GLOB WHEEL_FILES RELATIVE \"${CMAKE_CURRENT_SOURCE_DIR}/dist\" \"${CMAKE_CURRENT_SOURCE_DIR}/dist/*.whl\")
    list(SORT WHEEL_FILES COMPARE NATURAL ORDER DESCENDING)
    list(GET WHEEL_FILES 0 pkg)

    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        message(STATUS \"Installing dist/\${pkg} via pip3...\")
        execute_process(COMMAND \${RUN_PREFIX} \"pip3 install ${pipopt} --force-reinstall ${useropt} ./dist/\${pkg}\" \${RUN_SUFFIX} WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
    else()
        execute_process(COMMAND ${CMAKE_COMMAND} -E rename ./${PYTHON_EXT}.bak rpcf/${PYTHON_EXT} WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\"
            WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
        )        
        execute_process(COMMAND bash -lc \"echo haha i am here;pwd;ls -l --color ./dist\"
            WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E
             copy_if_different ./dist/\${pkg} \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\" WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        execute_process(COMMAND bash blddpkg.sh \"${CMAKE_CURRENT_SOURCE_DIR}/../\" \"${CMAKE_INSTALL_FULL_LIBDIR}\" ./dist/\${pkg} WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
    endif()
")

# ==============================================================================
# 5. Uninstall Custom Operations
# ==============================================================================
add_custom_target(uninstall_python
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        if(NOT \"\$ENV{SUDO_USER}\" STREQUAL \"\")
            execute_process(COMMAND su -c \"pip3 uninstall -y ${pipopt} @PACKAGE_NAME@\" \$ENV{SUDO_USER} WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        else()
            execute_process(COMMAND pip3 uninstall -y ${pipopt} @PACKAGE_NAME@ WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        endif()
    endif()
")

