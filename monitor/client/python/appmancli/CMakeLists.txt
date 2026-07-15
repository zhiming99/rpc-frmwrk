cmake_minimum_required(VERSION 3.18)
#project(rpcf_appmancli_py LANGUAGES NONE)

# Expose standard directory variables if needed
include(GNUInstallDirs)

set(mon_dir "${CMAKE_SOURCE_DIR}/monitor")
set(top_dir "${CMAKE_SOURCE_DIR}")
# ==============================================================================
# 1. Environment Introspection & Package Configuration (genpipopt & IS_VENV)
# ==============================================================================
execute_process(
    COMMAND python3 -c "import sys; print('true' if sys.prefix != sys.base_prefix else 'false')"
    OUTPUT_VARIABLE IS_VENV OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(IS_VENV STREQUAL "false")
    set(PYTHON_BUILD_CMD "python3" "-m" "build" "--wheel" "--no-isolation" "--outdir" "dist/" ".")
else()
    set(PYTHON_BUILD_CMD "pip3" "wheel" "." "--no-deps" "--no-build-isolation" "-w" "dist/")
endif()

cmake_host_system_information(RESULT OS_NAME QUERY DISTRIB_ID)
cmake_host_system_information(RESULT OS_VERSION QUERY DISTRIB_VERSION_ID)

# PEP 668 system protection overrides for modern Debian/Ubuntu baselines
set(pipopt "")

if(IS_VENV STREQUAL "false")
    if((OS_NAME STREQUAL "debian" OR OS_NAME STREQUAL "raspbian") AND OS_VERSION VERSION_GREATER_EQUAL "12")
        set(pipopt "--break-system-packages")
    elseif(OS_NAME STREQUAL "ubuntu" AND OS_VERSION VERSION_GREATER_EQUAL "23")
        set(pipopt "--break-system-packages")
    endif()
endif()

if(pipopt)
    message(STATUS "Warning: break-system-packages enabled in directory python/appmancli")
endif()

set(useropt "--no-deps")
if(IS_VENV STREQUAL "false")
    set(useropt "${useropt} --user")
endif()

# ==============================================================================
# 2. Automated IDL Interface Generation Rule (appmancli/__init__.py:)
# ==============================================================================
set(GENERATED_PY_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/appmancli/__init__.py"
    "${CMAKE_CURRENT_BINARY_DIR}/appmancli/AppManagerclibase.py"
    "${CMAKE_CURRENT_BINARY_DIR}/appmancli/appmonstructs.py"
    "${CMAKE_CURRENT_BINARY_DIR}/appmancli/AppManagercli.py"
    "${CMAKE_CURRENT_BINARY_DIR}/appmancli/maincli.py"
)

set(RIDLC_COMMAND "ridlc --sync_mode IAppStore=sync --sync_mode IDataProducer=sync --services=AppManager --client -pslO .  ${mon_dir}/appmon/appmon.ridl" )
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/appmancli")

add_custom_command(
    OUTPUT ${GENERATED_PY_FILES}
    
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/etc/rpcf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${top_dir}/ridl/seribase.py"
        "${CMAKE_BINARY_DIR}/etc/rpcf/"

    # Stage boilerplate code-generation tracking tools securely
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${top_dir}/ridl/mktpl" "${top_dir}/ridl/mktplsvr" 
        "${top_dir}/ridl/pymktpl" "${top_dir}/ridl/mktplcli" 
        "${top_dir}/ridl/drvtpl.json" "${top_dir}/ridl/odesctpl2.json" 
        "${top_dir}/ridl/synccfg.py"
        "${CMAKE_CURRENT_BINARY_DIR}/appmancli/"
        
    # Execute internal IDL tool compiler mapping environments
    COMMAND $<TARGET_FILE:ridlc> --sync_mode IAppStore=sync --sync_mode IDataProducer=sync --services=AppManager --client -pslO .  "${mon_dir}/appmon/appmon.ridl"
    DEPENDS "${mon_dir}/appmon/appmon.ridl"
        
    # override
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/appmancli/AppManagercli.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/appmancli/maincli.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/appmancli/apptimer.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/appmancli/"

    # Cleanup trailing tracking files
    COMMAND ${CMAKE_COMMAND} -E rm -f 
        mktpl pymktpl mktplsvr mktplcli odesctpl2.json drvtpl.json 
        *.new appmondesc.json driver.json *.md cmdline synccfg.py

    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/etc/rpcf"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/appmancli"
    COMMENT "Compiling Python AppMonitor interfaces via ridlc"
)

# Anchor target to verify interface dependencies execute correctly
add_custom_target(generate_appman_py_cli DEPENDS ${GENERATED_PY_FILES})
add_dependencies(generate_appman_py_cli ridlc combase ipc)

# ==============================================================================
# 3. Target "all" Execution Pipeline (Wheel Packaging)
# ==============================================================================
set(WHEEL_NAME "appmancli-${VERSION2}-py3-none-any.whl")
set(WHEEL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dist/${WHEEL_NAME}")

set(EXPORT_LICENSE "if ! grep 'dynamic = ' pyproject.toml; then python3 -c \"import setuptools; v=int(setuptools.__version__.split('.')[0]); f=open('pyproject.toml', 'a'); f.write('dynamic = [\\\"license\\\"]\\n') if v>=61 else True; f.close()\"; fi")

add_custom_command(
    OUTPUT "${WHEEL_OUTPUT}"
    # Synchronize setup templates into the workspace
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml" "${CMAKE_CURRENT_BINARY_DIR}/"
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/setup.py" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${top_dir}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE"
    
    # Process dynamic pyproject configurations matching modern requirements
    COMMAND bash -lc "${EXPORT_LICENSE}"
    VERBATIM
    # Compile the final distribution wheel packager target
    COMMAND ${PYTHON_BUILD_CMD}
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/LICENSE"
    
    # Export package to central tracking distributions workspace (../dist)
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/../dist"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp dist/appmancli*.whl ../dist/"
    
    DEPENDS ${GENERATED_PY_FILES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating Python appmancli Wheel package distribution"
)

add_custom_target(appmancli_wheel ALL DEPENDS "${WHEEL_OUTPUT}")
add_dependencies(appmancli_wheel generate_appman_py_cli )

# ==============================================================================
# 4. Installation Script Triggers (install: and uninstall:)
# ==============================================================================
install(CODE "
    # Skip deployment loops under fakeroot environments (e.g., dpkg creation runs)
    if(\"\$ENV{LD_LIBRARY_PATH}\" MATCHES \"libfakeroot\")
        message(STATUS \"Fakeroot environment detected: skipping explicit host pip installation loop.\")
    else()
        if(NOT \"\$ENV{SUDO_USER}\" STREQUAL \"\")
            set(RUN_PREFIX su -c)
            set(RUN_SUFFIX \" \$ENV{SUDO_USER}\")
        endif()

        file(GLOB INST_WHEELS RELATIVE \"${CMAKE_CURRENT_BINARY_DIR}/dist\" \"${CMAKE_CURRENT_BINARY_DIR}/dist/*.whl\")
        if(INST_WHEELS)
            list(SORT INST_WHEELS COMPARE NATURAL ORDER DESCENDING)
            list(GET INST_WHEELS 0 TARGET_WHEEL_PATH)
            if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
                message(STATUS \"Installing dist/\${TARGET_WHEEL_PATH} via pip3...\")
                execute_process(COMMAND \${RUN_PREFIX} \"pip3\" \"install\" \"${pipopt}\" \"--force-reinstall\" \"${useropt}\" \"dist/\${TARGET_WHEEL_PATH}\" \${RUN_SUFFIX} WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\")
            else()
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E make_directory \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\"
                    WORKING_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}\"
                )        
                execute_process(COMMAND ${CMAKE_COMMAND} -E
                     copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/dist/\${TARGET_WHEEL_PATH} \"\$ENV{DESTDIR}/${CMAKE_INSTALL_FULL_LIBDIR}/rpcf/\" WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\")
            endif()
        endif()
    endif()
")

add_custom_target(uninstall_python_appmancli
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    if(NOT \"\$ENV{SUDO_USER}\" STREQUAL \"\")
        execute_process(COMMAND su -c \"pip3 uninstall -y ${pipopt} appmancli\" \$ENV{SUDO_USER} WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\")
    else()
        execute_process(COMMAND pip3 uninstall -y ${pipopt} appmancli WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}\")
    endif()
")

