cmake_minimum_required(VERSION 3.18)
#project(rpcf_js LANGUAGES NONE) # No C/C++ compilation needed here

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Source Layout Collections (From Part 1 and Part 2)
# ==============================================================================
set(jslib_files
    rpc/protopkt.js
    combase/configdb.js
    combase/factory.js
    combase/msgmatch.js
    combase/objbase.js
    combase/cbuffer.js
    combase/defines.js
    combase/enums.js
    combase/SERI_HEADER_BASE.js
    dbusmsg/LICENSE
    dbusmsg/align.js
    dbusmsg/constants.js
    dbusmsg/dbus-buffer.js
    dbusmsg/header-signature.json
    dbusmsg/marshall.js
    dbusmsg/marshallers.js
    dbusmsg/message.js
    dbusmsg/signature.js
    dbusmsg/unmarshall.js
    rpc/bridge.js
    rpc/dmsg.js
    combase/iomsg.js
    rpc/ioworker.js
    rpc/handshak.js
    rpc/enablevtrmt.js
    ipc/enablevt.js
    ipc/iomgr.js
    ipc/proxy.js
    rpc/fwrdreqrmt.js
    ipc/fwrdreq.js
    combase/seribase.js
    rpc/fwrdevtrmt.js
    ipc/fwrdevt.js
    ipc/cancelrq.js
    ipc/keepalive.js
    rpc/keepalivermt.js
    ipc/openstm.js
    rpc/openstmrmt.js
    rpc/stream.js
    ipc/stmwrite.js
    ipc/stmevt.js
    rpc/stmwritermt.js
    rpc/lz4.js
    ipc/fastrpc.js
    ipc/login.js
    ipc/chkrtpath.js
    rpc/chkrtpathrmt.js
)

set(jslibdir "${CMAKE_INSTALL_FULL_LIBDIR}/rpcf")
set(archive_name "rpcbase.js.tar.gz")

# ==============================================================================
# 2. Archive Production Custom Commands (rpcbase.js.tar.gz:)
# ==============================================================================
# Generates the tarball locally inside the build binary folder
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${archive_name}"
    COMMAND ${CMAKE_COMMAND} -E tar "zcf" "${CMAKE_CURRENT_BINARY_DIR}/${archive_name}" ${jslib_files}
    DEPENDS ${jslib_files}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Packaging JavaScript libraries into ${archive_name}"
)

# Drive the file compilation rule on global target generation phases
add_custom_target(rpcf_js_bundle ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${archive_name}")

# ==============================================================================
# 3. Dynamic Installation Operations (install-data-hook:)
# ==============================================================================
# Deploys and unpacks the component tarball archive layout inside target trees
install(CODE "
    # Resolve staging destination prefix directories
    set(STAGED_JS_DIR \"\$ENV{DESTDIR}${jslibdir}/jslib\")
    
    # Replicates: mkdir -p $(DESTDIR)$(jslibdir)/jslib
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory \${STAGED_JS_DIR})
    
    # Replicates: tar -zxf ... -C ...
    message(STATUS \"Extracting ${archive_name} to \${STAGED_JS_DIR}\")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar \"zxf\" \"${CMAKE_CURRENT_BINARY_DIR}/${archive_name}\"
        WORKING_DIRECTORY \${STAGED_JS_DIR}
    )
    
    # Replicates: chown \`id -u\`:\`id -g\` -R
    # CMake's internal extract tool automatically maintains standard host user privileges, 
    # but we can shell call an explicit chown sequence if compiling on Unix environments.
    if(UNIX)
        execute_process(COMMAND id -u OUTPUT_VARIABLE CURRENT_UID OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND id -g OUTPUT_VARIABLE CURRENT_GID OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND chown -R \${CURRENT_UID}:\${CURRENT_GID} \${STAGED_JS_DIR})
    endif()
")

# ==============================================================================
# 4. Global Uninstallation Framework (uninstall-hook:)
# ==============================================================================
add_custom_target(uninstall_js
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    set(TARGET_JSLIB_DIR \"\$ENV{DESTDIR}${jslibdir}/jslib\")
    if(EXISTS \"\${TARGET_JSLIB_DIR}\" AND IS_DIRECTORY \"\${TARGET_JSLIB_DIR}\")
        message(STATUS \"Removing JavaScript distribution library workspace: \${TARGET_JSLIB_DIR}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -rf \"\${TARGET_JSLIB_DIR}\")
    endif()
")

