cmake_minimum_required(VERSION 3.18)
project(rpcf_rpcrouter LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir, mandir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables and Architecture Macros Mapping
# ==============================================================================
set(include_rpcdir "${PROJECT_SOURCE_DIR}/rpc")
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")
set(include_fusedir "${PROJECT_SOURCE_DIR}/fuse")

set(OFFSET_MACRO "")
if(BITWIDTH32)
    set(OFFSET_MACRO "-D_FILE_OFFSET_BITS=64")
endif()

# Gather common include paths for cleaner macro mapping
set(ROUTER_COMMON_INCLUDES
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${include_rpcdir}"
    "${include_ipcdir}"
)

# ==============================================================================
# 2. Shared Library Target (librtfiles.la)
# ==============================================================================
set(librtfiles_la_SOURCES 
    rtdump.cpp 
    rtappman.cpp 
    rtappman.h
)

if(ENABLE_FUSE3)
    list(APPEND librtfiles_la_SOURCES rtfiles.cpp rtfiles.h)
endif()

# Replaces: librtfiles_la_LTLIBRARIES=librtfiles.la
add_library(rtfiles SHARED ${librtfiles_la_SOURCES})

# Replaces: librtfiles_la_CPPFLAGS
target_include_directories(rtfiles PRIVATE 
    ${ROUTER_COMMON_INCLUDES}
    "${PROJECT_SOURCE_DIR}/monitor/client/cpp/appmancli"
)
set_target_properties(rtfiles PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OFFSET_MACRO)
    target_compile_definitions(rtfiles PRIVATE "${OFFSET_MACRO}")
endif()

if(ENABLE_FUSE3)
    target_include_directories(rtfiles PRIVATE ${include_fusedir})
    target_compile_definitions(rtfiles PRIVATE FUSE3)
endif()

# Replaces: librtfiles_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(rtfiles PROPERTIES 
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Replaces: librtfiles_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(rtfiles PRIVATE "${PROJECT_SOURCE_DIR}/3rd_party")
target_link_libraries(rtfiles PRIVATE
    appmancli     # Native targets reference mapping instead of physical .la paths
    rpc
    ipc
    combase
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

if(ENABLE_FUSE3)
    target_link_libraries(rtfiles PRIVATE fuseif) # Links native fuse target
endif()

# ==============================================================================
# 3. Core Router Program Target (rpcrouter)
# ==============================================================================
# Replaces: rpcrouter_PROGRAMS=rpcrouter (rpcrouter.1 omitted from compilation elements)
add_executable(rpcrouter routmain.cpp routmain.h)

# Replaces: rpcrouter_CPPFLAGS
target_include_directories(rpcrouter PRIVATE ${ROUTER_COMMON_INCLUDES})
set_target_properties(rpcrouter PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside rpcrouter_LDFLAGS
)

if(OFFSET_MACRO)
    target_compile_definitions(rpcrouter PRIVATE "${OFFSET_MACRO}")
endif()

if(ENABLE_FUSE3)
    target_include_directories(rpcrouter PRIVATE ${include_fusedir})
    target_compile_definitions(rpcrouter PRIVATE FUSE3)
endif()

if(ENABLE_AUTH)
    target_compile_definitions(rpcrouter PRIVATE AUTH)
endif()

# Replaces: rpcrouter_LDADD
target_link_directories(rpcrouter PRIVATE
    "${PROJECT_SOURCE_DIR}/combase"
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/3rd_party"
    "${PROJECT_SOURCE_DIR}/fuse"
)

target_link_libraries(rpcrouter PRIVATE
    rtfiles       # Local library reference dependency
    appmancli     # Cross-submodule target linking
    rpc           # Parent submodule target linking
    ipc
    combase
    utils
    stdc++
    cppunit
    ${COMMON_LINK}
)

if(ENABLE_FUSE3)
    target_link_libraries(rpcrouter PRIVATE fuseif)
endif()

# Establish target building sequence hierarchy securely
add_dependencies(rpcrouter rtfiles)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================
# Deploy main router executable binary target
# Replaces: rpcrouterdir=$(bindir)
install(TARGETS rpcrouter DESTINATION ${CMAKE_INSTALL_BINDIR})

# Deploy shared helper library binaries
# Replaces: librtfiles_ladir=$(libdir)/rpcf
install(TARGETS rtfiles DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Deploy json system configuration files
# Replaces: rpcrouterconfdir=$(sysconfdir)/rpcf / dist_rpcrouterconf_DATA
install(FILES router.json rtauth.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

# Deploy UNIX Manual Reference Pages
# Replaces: man_MANS=rpcrouter.1
install(FILES rpcrouter.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

