cmake_minimum_required(VERSION 3.18)
#project(rpcf_oinit LANGUAGES NONE)

# Include standard directory layout configurations (maps mandir, libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Environment Introspection & Packaging Command Variable Mapping
# ==============================================================================
# Replaces: IS_VENV := $(shell python3 ...)
execute_process(
    COMMAND python3 -c "import sys; print('true' if sys.prefix != sys.base_prefix else 'false')"
    OUTPUT_VARIABLE IS_VENV
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Replaces: command := $(if $(filter false,$(IS_VENV)),...)
if(IS_VENV STREQUAL "false")
    set(PYTHON_BUILD_CMD "python3 -m build --wheel --no-isolation --outdir dist/ .")
    set(PIP_INSTALL_OPTS "--force-reinstall --user --break-system-packages")
else()
    set(PYTHON_BUILD_CMD "pip3 wheel . --no-deps --no-build-isolation -w dist/")
    set(PIP_INSTALL_OPTS "")
endif()

# ==============================================================================
# 2. Target "wheel" Custom Operation
# ==============================================================================
add_custom_target(wheel ALL
    # 1. Clear out stale output layers safely using native directory-wipes
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg"

    # 2. Re-initialize a clean staging workspace tree
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit"

    # 3. Securely stage explicit static file definitions
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/setup.py" "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/oinitdesc.json" "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/"

    COMMAND bash -c "cp ${CMAKE_CURRENT_SOURCE_DIR}/*.py ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/ && cp ${CMAKE_CURRENT_SOURCE_DIR}/README* ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/ && rm -f ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/setup.py"

    # 5. THE FIX: Safe Compilation and Extraction Pipeline
    COMMAND bash -c "cd ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg && ${PYTHON_BUILD_CMD} && cp dist/*.whl ${CMAKE_CURRENT_BINARY_DIR}/output/"

    # 6. Clean up temporary packaging staging artifacts immediately following extraction
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg"
    COMMAND "${CMAKE_COMMAND}" -E echo "Success: Python wheel package successfully generated in 'output' directory."

    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Compiling Python Wheel packages via isolated environment wrappers..."
    VERBATIM # Ensures all quotes and spaces pass exactly as intended to the OS
)

# ==============================================================================
# 3. Target "deb" Custom Operation
# ==============================================================================
add_custom_target(deb_oinit
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/debpkg"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian"
    COMMAND ${CMAKE_COMMAND} -E echo "10" > "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/compat"
    COMMAND ${CMAKE_COMMAND} -E echo "usr/lib/rpcf/*.whl" > "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/python3-oinit.install"
    
    # Synchronize debian scripts layout
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp -r ${CMAKE_CURRENT_SOURCE_DIR}/debian/* ${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_CURRENT_SOURCE_DIR}/*.py ${CMAKE_CURRENT_SOURCE_DIR}/*.md ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/"
    COMMAND bash -lc "cp ${CMAKE_CURRENT_SOURCE_DIR}/oinitdesc.json ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/"
    COMMAND bash -lc "cp ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.deb ${CMAKE_CURRENT_BINARY_DIR}/debpkg/Makefile" 
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/setup.py ${CMAKE_CURRENT_BINARY_DIR}/debpkg/ || true"
    
    # Execute dpkg-buildpackage pipelines
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd ${CMAKE_CURRENT_BINARY_DIR}/debpkg && DEBEMAIL=\"${PACKAGE_BUGREPORT}\" DEBFULLNAME=\"zhiming\" dch --create -v ${VERSION}-${REV} --package python3-oinit \"python3-oinit-${VERSION}-${REV} released\" && dpkg-buildpackage -us -uc"
    VERBATIM
    
    # Consolidate outputs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/python3-oinit_${VERSION2}-${REV}_*.deb ${CMAKE_CURRENT_BINARY_DIR}/output/"
    COMMAND ${CMAKE_COMMAND} -E echo "deb package is generated in 'output' directory."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Debian packages (.deb)"
)

# ==============================================================================
# 4. Target "rpm" Custom Operation
# ==============================================================================
add_custom_target(rpm_oinit
    DEPENDS wheel
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/oinitrpm"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/BUILD"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/BUILDROOT"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/RPMS"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SOURCES"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SPECS"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SRPMS"
    
    # Deploy source wheel to SOURCES root
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_CURRENT_BINARY_DIR}/output/*.whl $ENV{HOME}/oinitrpm/SOURCES/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/oinit.spec "$ENV{HOME}/oinitrpm/SPECS/"
    
    # Run rpmbuild configurations
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd $ENV{HOME}/oinitrpm && rpmbuild -bb --define \"_topdir \`pwd\`\" SPECS/oinit.spec && cp ./RPMS/noarch/*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/oinitrpm"
    
    # Consolidate output targets
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "rm -f ${CMAKE_CURRENT_BINARY_DIR}/output/python3-oinit*rpm || true"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/python3-oinit*.rpm ${CMAKE_CURRENT_BINARY_DIR}/output/ || true"
    COMMAND ${CMAKE_COMMAND} -E echo "rpm package is generated in 'output' directory."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating RedHat packaging distributions (.rpm)"
)

# ==============================================================================
# 5. Target "localinst" Custom Operation
# ==============================================================================
add_custom_target(localinst
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target wheel
    COMMAND ${CMAKE_COMMAND} -E env bash -c "pip3 install ${PIP_INSTALL_OPTS} ${CMAKE_CURRENT_BINARY_DIR}/output/*.whl"
    COMMENT "Installing python packages locally via pip3"
)

# ==============================================================================
# 6. Standard Installation Hook Layout Definitions
# ==============================================================================
if(IS_VENV STREQUAL "false")
    set(useropt "--user")
else()
    set(useropt "")
endif()

set(pipopt "")
if(IS_VENV STREQUAL "false")
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
endif()

install(FILES oinit.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(CODE "
    # Recursively find the file generated by your custom target step above
    file(GLOB INST_WHEELS RELATIVE \"${CMAKE_CURRENT_BINARY_DIR}/output\" \"${CMAKE_CURRENT_BINARY_DIR}/output/*.whl\")
    
    # User Privilege Management Blocks 
    if(NOT \"\$ENV{SUDO_USER}\" STREQUAL \"\")
        set(RUN_PREFIX su -c)
        set(RUN_SUFFIX \"\$ENV{SUDO_USER}\")
    endif()

    if(INST_WHEELS)
	list(SORT INST_WHEELS COMPARE NATURAL ORDER DESCENDING)
        list(GET INST_WHEELS 0 TARGET_WHEEL_PATH)
        message(STATUS \"Found wheel target for packaging/installation: \${TARGET_WHEEL_PATH}\")
        
        # Insert your standard dynamic DESTDIR/CPack redirection block here...

        if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
            message(STATUS \"Installing dist/\${TARGET_WHEEL_PATH} via pip3...\")
            execute_process(COMMAND \${RUN_PREFIX} \"pip3 install ${pipopt} --force-reinstall ${useropt} ${CMAKE_CURRENT_BINARY_DIR}/output/\${TARGET_WHEEL_PATH}\" \${RUN_SUFFIX} WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        else()
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E make_directory \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\"
                WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
            )        
            execute_process(COMMAND ${CMAKE_COMMAND} -E
                 copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/output/\${TARGET_WHEEL_PATH} \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\" WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\")
        endif()
    endif()
")
