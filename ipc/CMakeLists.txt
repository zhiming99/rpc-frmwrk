# 1. --- Define Source Files (libipc_la_SOURCES) ---
set(IPC_SOURCES
    loopool.cpp stream.cpp portex.cpp iftasks.cpp ifstat.cpp
    rpcif.cpp sevpoll.cpp counters.cpp
    connmgr.cpp filetran.cpp mainloop.cpp dllexp.cpp
    evloop.cpp dbushook.cpp irp.cpp irpthread.cpp
    port.cpp portstat.cpp drvmgr.cpp utils.cpp
    iomgr.cpp ptifmap.cpp pnpmgr.cpp dbusdrv.cpp
    dbusport.cpp portdrv.cpp rpcpdo.cpp dbuspdo.cpp
    tasklets.cpp lpbkmtch.cpp ifhelper.cpp iiddict.cpp
    reqopen.cpp uxport.cpp uxstream.cpp streamex.cpp
    prxyfdo.cpp dbusprxy.cpp fdodrv.cpp fastrpc.cpp
    stmport.cpp msgmatch.cpp stmcp.cpp tractgrp.cpp
)

# 2. --- Create the Library Target (lib_LTLIBRARIES) ---
# Automatically compiles as static or shared based on BUILD_SHARED_LIBS
add_library(ipc ${IPC_SOURCES})

# 3. --- Define Public Header Install Group (rpcfipc_HEADERS) ---
# Prepend the directory path since they are located in $(top_srcdir)/include
set(RPCF_IPC_HEADERS
    "../include/loopool.h"    "../include/connhelp.h"   "../include/counters.h"
    "../include/dbusport.h"   "../include/emaphelp.h"   "../include/evloop.h"
    "../include/filetran.h"   "../include/frmwrk.h"     "../include/ifhelper.h"
    "../include/ifstat.h"     "../include/iftasks.h"    "../include/iiddict.h"
    "../include/irp.h"        "../include/jsondef.h"    "../include/mainloop.h"
    "../include/msgmatch.h"   "../include/namespc.h"    "../include/port.h"
    "../include/portdrv.h"    "../include/portex.h"     "../include/reqopen.h"
    "../include/sevpoll.h"    "../include/stream.h"     "../include/streamex.h"
    "../include/tasklets.h"   "../include/tractgrp.h"   "../include/utils.h"
    "../include/uxport.h"     "../include/uxstream.h"   "../include/proxy.h"
    "../include/prxyport.h"   "../include/fdodrv.h"     "../include/stmport.h"
    "../include/fastrpc.h"    "../include/stmcp.h"      "../include/seqtgmgr.h"
)

# 4. --- Handle Include Directories (libipc_la_CPPFLAGS) ---
# Replaces -I$(top_srcdir)/logger. We include the root directory as well 
# so your header template config.h generated inside the build folder can be seen.
target_include_directories(ipc PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/../logger"
    "${CMAKE_BINARY_DIR}"
)

# 5. --- Library Target Properties & Versioning (libipc_la_LDFLAGS) ---
# Parses your colon-separated VERSION2 tracking string into a clean layout properties map
string(REPLACE ":" "." SO_VERSION "${VERSION2}")

set_target_properties(ipc PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${SO_VERSION}"
    OUTPUT_NAME "ipc"
)

# 6. --- Link Dependencies & Target Libraries (libipc_la_LIBADD) ---
# We bypass relative path lookup variables entirely and bind straight to the 'combase' target.
# It automatically bubbles up its dependencies (like lz4, jsoncpp, threads, dl) cleanly.
target_link_libraries(ipc PRIVATE
    combase
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# 7. --- Installation Rules (libipc_ladir, rpcfipcdir, and dist_ipcconf_DATA) ---
include(GNUInstallDirs)

# Installs the generated libipc binary target layout configurations
install(TARGETS ipc
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Installs the network core include assets to $(includedir)/rpcf/ipc
install(FILES ${RPCF_IPC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/ipc
)

# Installs driver configuration assets (dist_ipcconf_DATA) to $(sysconfdir)/rpcf
install(FILES driver.json
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
)
