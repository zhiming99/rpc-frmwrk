cmake_minimum_required(VERSION 3.18)
project(rpcf_sslport LANGUAGES C CXX)

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Shared Library Target (libsslpt.la)
# ==============================================================================
set(libsslpt_la_SOURCES
    dllexp.cpp
    sslfido.cpp
    sslutils.cpp
    sslfido.h
)

# Replaces: lib_LTLIBRARIES=libsslpt.la
add_library(sslpt SHARED ${libsslpt_la_SOURCES})

# Replaces: libsslpt_la_CPPFLAGS
# Native CMake cross-compilation configurations automatically handle the CMAKE_SYSROOT 
# target paths, meaning we don't need to manually inject $(SYSROOT)/usr/include.
target_include_directories(sslpt PRIVATE
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/combase"
)

set_target_properties(sslpt PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libsslpt_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(sslpt PROPERTIES
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Replaces: libsslpt_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(sslpt PRIVATE
    "${PROJECT_SOURCE_DIR}/combase"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/3rd_party"
)

target_link_libraries(sslpt PRIVATE
    rpc          # Native target tokens instead of physical .la text path targets
    ipc
    combase
    ${COMMON_LINK_SSL} # Inherited OpenSSL link variables from master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 2. Installation Routines
# ==============================================================================
# Replaces: libsslpt_ladir=$(libdir)
install(TARGETS sslpt DESTINATION ${CMAKE_INSTALL_LIBDIR})

