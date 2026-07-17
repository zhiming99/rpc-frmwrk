cmake_minimum_required(VERSION 3.18)
#project(rpcf_tools LANGUAGES NONE) # Scripting & utility target, no direct C/C++ compilation

# Expose standard directory layout variables (maps bindir, libdir, sysconfdir, localedir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (From Part 1 and Part 2)
# ==============================================================================
set(pysources
    rpcfg.py rpcfgnui.py updinitcfg.py updwscfg.py krbparse.py
    updk5cfg.py disablessl.py geninitcfg.py buildinst.py unzipinst.sh rpcfgtui.py
)

set(runtime_files "")

# ==============================================================================
# 2. Localized Text Translation Target (rpcfgtui.mo:)
# ==============================================================================
find_program(MSGFMT_EXECUTABLE msgfmt)
if(MSGFMT_EXECUTABLE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rpcfgtui.mo"
        COMMAND ${MSGFMT_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/rpcfgtui-zh_CN.po" -o "${CMAKE_CURRENT_BINARY_DIR}/rpcfgtui.mo"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rpcfgtui-zh_CN.po"
        COMMENT "Compiling translation catalogs via msgfmt"
    )
    add_custom_target(compile_locales ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/rpcfgtui.mo")
    
    # Replaces: locale_rpcfgtui_DATA=rpcfgtui.mo / locale_rpcfgtuidir=$(localedir)/zh_CN/LC_MESSAGES
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rpcfgtui.mo" DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/zh_CN/LC_MESSAGES")
else()
    message(WARNING "msgfmt executable not detected. System translation generation will be bypassed.")
endif()

# ==============================================================================
# 3. Auxiliary Package Custom Operations (docker:)
# ==============================================================================
add_custom_target(docker
    COMMAND docker build -t rpcf .
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building system Docker container image (rpcf)"
)

# ==============================================================================
# 4. Installation Paradigms (install-exec-local:)
# ==============================================================================

# Emulate runtest.sh template substitutions
# Replicates: sed -e 's|[@]bindir[@]... runtest.sh.in > runtest.sh
# Using configuration file generation strategies dynamically loops values into binary target locations
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/runtest.sh.in"
    "${CMAKE_CURRENT_BINARY_DIR}/runtest.sh"
    @ONLY
)

# Deploy primary Python utilities package bundle into absolute destinations pointer (${bindir}/rpcf)
install(PROGRAMS
    rpcfg.py
    rpcfgnui.py
    rpcfgtui.py
    DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/updcfg.py
    updinitcfg.py
    updwscfg.py
    krbparse.py
    updk5cfg.py
    disablessl.py
    geninitcfg.py
    buildinst.py
    unzipinst.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf
)

# Handle platform-specific security variables conditionally matching evaluation strings
if(ENABLE_GMSSL)
    list(APPEND runtime_files "gmsslkey.sh")
    install(PROGRAMS gmsslkey.sh DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)
endif()

if(ENABLE_OPENSSL)
    list(APPEND runtime_files "opensslkey.sh")
    install(PROGRAMS opensslkey.sh DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)
    
    # Replaces: dist_testcfg_DATA=testcfgs/openssl.cnf to testcfgdir=$(sysconfdir)/rpcf
    install(FILES testcfgs/openssl.cnf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)
endif()

# ==============================================================================
# 5. Global Uninstallation Operations (uninstall-hook:)
# ==============================================================================
add_custom_target(uninstall_tools
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

# Write structural helper clean targets code configurations natively
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    set(TARGET_TOOLS_DIR \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/rpcf\")
    if(EXISTS \"\${TARGET_TOOLS_DIR}\" AND IS_DIRECTORY \"\${TARGET_TOOLS_DIR}\")
        message(STATUS \"Removing tools workspace directory layer: \${TARGET_TOOLS_DIR}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf \"\${TARGET_TOOLS_DIR}\")
    endif()
")

