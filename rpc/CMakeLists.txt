cmake_minimum_required(VERSION 3.18)
project(rpcf_rpc LANGUAGES C CXX)

# Expose standard directory variables (maps libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (librpc.la)
# ==============================================================================
set(rpc_headers
    conntask.h
    stmrelay.h
    streammh.h
    tcportex.h
    tcpport.h
    rpcroute.h
    taskschd.h
    readme.md
    rtseqmgr.h
)

set(librpc_la_SOURCES
    taskschd.cpp
    ldbal.cpp
    tcpport.cpp
    stmpdo.cpp
    tcpfido.cpp
    reqfwdr.cpp
    rpcroute.cpp
    bridge.cpp
    dllexp.cpp
    fidodrv.cpp
    stmrelay.cpp
    tcportex.cpp
    stmpdo2.cpp
    multihop.cpp
    streammh.cpp
    ${rpc_headers}
)

# Replaces: lib_LTLIBRARIES=librpc.la
add_library(rpc SHARED ${librpc_la_SOURCES})

# Replaces: librpc_la_CPPFLAGS = -I$(include_ipcdir)
target_include_directories(rpc PRIVATE ${include_ipcdir})
set_target_properties(rpc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: librpc_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(rpc PROPERTIES
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Replaces: librpc_la_LIBADD= ... -lpthread -ldl
find_package(Threads REQUIRED)
target_link_directories(rpc PRIVATE
    "${PROJECT_SOURCE_DIR}/combase"
    "${PROJECT_SOURCE_DIR}/ipc"
)
target_link_libraries(rpc PRIVATE
    combase # Target reference name instead of .la links
    ipc     # Target reference name instead of .la links
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Installation Routines (Assets & Targets)
# ==============================================================================
# Deploy shared library binary to libdir
install(TARGETS rpc DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Replaces: dist_rpcconf_DATA=security/authprxy.json to rpcconfdir=$(sysconfdir)/rpcf
install(FILES security/authprxy.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

# ==============================================================================
# 4. Nested Directory Execution Control Sequence (SUBDIRS Mapping)
# ==============================================================================
# Standard subdirectory router mappings
add_subdirectory(router)

# Conditional subdirectory routers mapping block
if(ENABLE_GMSSL)
    add_subdirectory(gmsslport)
endif()

if(ENABLE_OPENSSL)
    add_subdirectory(sslport)
    add_subdirectory(wsport)
else()
    if(ENABLE_GMSSL)
        add_subdirectory(wsport)
    endif()
endif()

# Replaces: SUBDIRS += security (Unconditional as per current Makefile state)
add_subdirectory(security)

