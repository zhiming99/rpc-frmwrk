cmake_minimum_required(VERSION 3.18)
project(rpcf_wsport LANGUAGES C CXX)

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Nested Directory Execution Control (SUBDIRS= wsproto)
# ==============================================================================
add_subdirectory(wsproto)

# ==============================================================================
# 2. Shared Library Target (libwspt.la)
# ==============================================================================
set(libwspt_la_SOURCES
    dllexp.cpp
    wsfido.cpp
    wsfido.h
    Readme.md
)

# Replaces: lib_LTLIBRARIES=libwspt.la
add_library(wspt SHARED ${libwspt_la_SOURCES})

# Replaces: libwspt_la_CPPFLAGS
target_include_directories(wspt PRIVATE
    "${CMAKE_SOURCE_DIR}/ipc"
    "${CMAKE_SOURCE_DIR}/rpc"
    "${CMAKE_SOURCE_DIR}/combase"
)

set_target_properties(wspt PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libwspt_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(wspt PROPERTIES
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Replaces: libwspt_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(wspt PRIVATE
    "${CMAKE_SOURCE_DIR}/combase"
    "${CMAKE_SOURCE_DIR}/rpc"
    "${CMAKE_SOURCE_DIR}/ipc"
    "${CMAKE_SOURCE_DIR}/3rd_party"
    "${CMAKE_CURRENT_SOURCE_DIR}/wsproto"
)

target_link_libraries(wspt PRIVATE
    rpc          # Native target tokens instead of physical .la text path targets
    ipc
    combase
    wsock
    utils
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

# ==============================================================================
# 3. Installation Routines
# ==============================================================================
# Replaces: libwspt_ladir=$(libdir)
install(TARGETS wspt DESTINATION ${CMAKE_INSTALL_LIBDIR})


