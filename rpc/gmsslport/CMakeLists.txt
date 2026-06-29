cmake_minimum_required(VERSION 3.18)
project(rpcf_gmsslport LANGUAGES C CXX)

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Shared Library Target (libgmsslpt.la)
# ==============================================================================
set(libgmsslpt_la_SOURCES
    dllexp.cpp
    gmsfido.cpp
    agmsapi.cpp
    agmsapi.h
    gmsfido.h
    README_cn.md
)

# Replaces: lib_LTLIBRARIES=libgmsslpt.la
add_library(gmsslpt SHARED ${libgmsslpt_la_SOURCES})

# Replaces: libgmsslpt_la_CPPFLAGS
# Automatically handles compiler search contexts for system header environments using cross-compilation toolchains
target_include_directories(gmsslpt PRIVATE
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/combase"
    "/usr/local/include"
)

# Apply explicit platform cryptographic macro toggles
target_compile_definitions(gmsslpt PRIVATE
    ENABLE_SHA1
    ENABLE_SHA2
)

set_target_properties(gmsslpt PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libgmsslpt_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(gmsslpt PROPERTIES
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Replaces: libgmsslpt_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(gmsslpt PRIVATE
    "${PROJECT_SOURCE_DIR}/combase"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/3rd_party"
    "/usr/local/lib"
)

target_link_libraries(gmsslpt PRIVATE
    rpc          # Native target token resolution instead of physical .la assets path strings
    ipc
    combase
    gmssl        # Links system GMSSL cryptography library
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 2. Installation Routines
# ==============================================================================
# Replaces: libgmsslpt_ladir=$(libdir)
install(TARGETS gmsslpt DESTINATION ${CMAKE_INSTALL_LIBDIR})

