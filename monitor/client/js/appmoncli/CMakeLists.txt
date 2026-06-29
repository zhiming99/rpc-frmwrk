cmake_minimum_required(VERSION 3.18)
project(rpcf_appmoncli_js LANGUAGES NONE)

# Expose standard directory variables (maps sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(ridl_file "${PROJECT_SOURCE_DIR}/monitor/appmon/appmon.ridl")
set(archive_name "appmonui.tar.gz")

set(auto_files
    "appmonstructs.js"
    "AppMonitorclibase.js"
    "driver.json"
    "appmondesc.json"
)

set(files_to_package
    "appmon.html"
    "locales/zh.json"
    "locales/en.json"
    "i18n-helper.js"
    "appdetail.js"
    "appmondesc.json"
)

# ==============================================================================
# 2. Automated IDL Interface Generation Rule (ridl_file custom command)
# ==============================================================================
# Maps the explicit file outputs produced by the ridlc tool execution step
set(GENERATED_JS_FILES "")
foreach(_file ${auto_files})
    list(APPEND GENERATED_JS_FILES "${CMAKE_CURRENT_BINARY_DIR}/${_file}")
endforeach()

add_custom_command(
    OUTPUT ${GENERATED_JS_FILES}
    # Stage boilerplate code-generation templates securely into the binary directory
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/ridl/mktpl" "${PROJECT_SOURCE_DIR}/ridl/mktplsvr" 
        "${PROJECT_SOURCE_DIR}/ridl/mktplcli" "${PROJECT_SOURCE_DIR}/ridl/drvtpl.json" 
        "${PROJECT_SOURCE_DIR}/ridl/odesctpl2.json" "${PROJECT_SOURCE_DIR}/ridl/synccfg.py"
        "${CMAKE_CURRENT_BINARY_DIR}/"
        
    # Execute internal IDL tool compiler mapping environments
    COMMAND ${CMAKE_COMMAND} -E env 
        LD_LIBRARY_PATH="${PROJECT_BINARY_DIR}/combase:${PROJECT_SOURCE_DIR}/combase:\$ENV{LD_LIBRARY_PATH}"
        "${PROJECT_SOURCE_DIR}/ridl/ridlc" 
        --noreadme --services=AppMonitor -sJO . "${ridl_file}"
        
    # Clean up intermediate configuration logs
    COMMAND ${CMAKE_COMMAND} -E rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json
    
    # Run downstream profile data updating wrapper
    COMMAND python3 "${PROJECT_SOURCE_DIR}/monitor/appmon/upddesc.py"
    
    DEPENDS "${ridl_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling JavaScript AppMonitor IDL Interfaces via ridlc"
)

add_custom_target(generate_appmon_js_cli DEPENDS ${GENERATED_JS_FILES})

# ==============================================================================
# 3. Webpack Compilation Pipeline Target (dist/appmon.js)
# ==============================================================================
# Replicates: dist/appmon.js: $(auto_files) maincli.js AppMonitorcli.js
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dist/appmon.js"
    # Replicate tracking configurations from source into binary playground
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.skel" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/webpack.config.js" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/maincli.js" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/AppMonitorcli.js" "${CMAKE_CURRENT_BINARY_DIR}/"
    
    # Replicates condition: if echo $(CPPFLAGS) | grep 'DDEBUG'; then ...
    COMMAND ${CMAKE_COMMAND} -E env bash -c "
        if [[ \"\${CMAKE_BUILD_TYPE}\" == \"Debug\" ]]; then
            make -f Makefile.skel debug;
        else
            make -f Makefile.skel release;
        fi
    "
    DEPENDS ${GENERATED_JS_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/maincli.js" "${CMAKE_CURRENT_SOURCE_DIR}/AppMonitorcli.js"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling JavaScript distribution web bundle via Webpack"
)

add_custom_target(appmon_js_bundle DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dist/appmon.js")
add_dependencies(appmon_js_bundle generate_appmon_js_cli)

# ==============================================================================
# 4. Target "all" Archive Construction (appmonui.tar.gz)
# ==============================================================================
set(TARBALL_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${archive_name}")

add_custom_command(
    OUTPUT "${TARBALL_OUTPUT}"
    # Synchronize asset configurations into compilation environment workspace
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/locales"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/appmon.html" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/i18n-helper.js" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/appdetail.js" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/locales/zh.json" "${CMAKE_CURRENT_BINARY_DIR}/locales/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/locales/en.json" "${CMAKE_CURRENT_BINARY_DIR}/locales/"
    
    # Replicates: tar zcf appmonui.tar.gz $(files_to_package) dist/*
    COMMAND ${CMAKE_COMMAND} -E env bash -c "tar zcf '${TARBALL_OUTPUT}' ${files_to_package} dist/*"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dist/appmon.js"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Packaging web user interface application into ${archive_name}"
)

# Global custom target wrapper to force inclusion inside default target blocks
add_custom_target(appmonui ALL DEPENDS "${TARBALL_OUTPUT}")
add_dependencies(appmonui appmon_js_bundle)

# ==============================================================================
# 5. Supplementary Packaging Targets (package & dbg-package)
# ==============================================================================
add_custom_target(package_appmonui
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.skel" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd '${CMAKE_CURRENT_BINARY_DIR}' && make -f Makefile.skel && tar zcf '${TARBALL_OUTPUT}' ${files_to_package} dist/*"
    DEPENDS generate_appmon_js_cli
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Executing production pipeline package bundle"
)

add_custom_target(dbg-package_appmonui
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.skel" "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd '${CMAKE_CURRENT_BINARY_DIR}' && make -f Makefile.skel debug && tar zcf '${TARBALL_OUTPUT}' ${files_to_package} dist/*"
    DEPENDS generate_appmon_js_cli
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Executing debug pipeline package bundle"
)

# ==============================================================================
# 6. Installation Routines
# ==============================================================================
# Replaces: appmonuiconfdir=$(sysconfdir)/rpcf / appmonuiconf_DATA=appmonui.tar.gz
install(FILES "${TARBALL_OUTPUT}" DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/rpcf")

