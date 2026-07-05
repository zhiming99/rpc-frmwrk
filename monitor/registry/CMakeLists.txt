cmake_minimum_required(VERSION 3.18)
#project(rpcf_registry LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir, includedir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(include_ipcdir "${CMAKE_SOURCE_DIR}/ipc")

# Handle 32-bit architecture offsets and macro blocks conditionally
set(OFFSET_MACRO "")
if(NOT ENABLE_FUSE3)
    # Kept empty as per legacy Makefile conditional order
else()
    if(BITWIDTH32)
        set(OFFSET_MACRO "-D_FILE_OFFSET_BITS=64")
    endif()
endif()

# ==============================================================================
# 2. Shared Library Target (libregfs.la)
# ==============================================================================
# Replaces: libregfs_la_LTLIBRARIES=libregfs.la
add_library(regfs SHARED
    regfs.cpp 
    blkalloc.h 
    blkalloc.cpp 
    dir.cpp 
    file.cpp 
    dllexp.cpp
)

# Replaces: libregfs_la_CPPFLAGS
target_include_directories(regfs PRIVATE ${include_ipcdir})
set_target_properties(regfs PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OFFSET_MACRO)
    target_compile_definitions(regfs PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: libregfs_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(regfs PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Replaces: libregfs_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(regfs PRIVATE "${CMAKE_BINARY_DIR}/3rd_party")
target_link_libraries(regfs PRIVATE
    combase # Target linking instead of .la asset references
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

# ==============================================================================
# 3. Executable Program Targets (regfsmnt & regfschk)
# ==============================================================================

# --- Setup Shared Dependencies / Variables for Binaries ---
set(BINARY_LIBS
    regfs
    combase
    utils
    stdc++
    cppunit
)

set(BINARY_LINK_DIRS
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
    "${CMAKE_BINARY_DIR}/3rd_party"
)

# --- Target: regfsmnt ---
add_executable(regfsmnt regfuse.cpp)
target_include_directories(regfsmnt PRIVATE ${include_ipcdir})
set_target_properties(regfsmnt PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic
)

if(OFFSET_MACRO)
    target_compile_definitions(regfsmnt PRIVATE "${OFFSET_MACRO}")
endif()

target_link_directories(regfsmnt PRIVATE ${BINARY_LINK_DIRS})
target_link_libraries(regfsmnt PRIVATE ${BINARY_LIBS})

# Handle FUSE3 compilation conditionals for regfsmnt
if(ENABLE_FUSE3)
    target_compile_definitions(regfsmnt PRIVATE FUSE3)
    target_link_libraries(regfsmnt PRIVATE ${COMMON_LINK_FUSE3})
else()
    target_link_libraries(regfsmnt PRIVATE ${COMMON_LINK})
endif()


# --- Target: regfschk ---
add_executable(regfschk regfschk.cpp)
target_include_directories(regfschk PRIVATE ${include_ipcdir})
set_target_properties(regfschk PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic
)

if(OFFSET_MACRO)
    target_compile_definitions(regfschk PRIVATE "${OFFSET_MACRO}")
endif()

target_link_directories(regfschk PRIVATE ${BINARY_LINK_DIRS})
target_link_libraries(regfschk PRIVATE ${BINARY_LIBS} ${COMMON_LINK})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libregfs_ladir=$(libdir)
install(TARGETS regfs DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Replaces: regfsmntdir=$(bindir), regfschkdir=$(bindir)
install(TARGETS regfsmnt regfschk DESTINATION ${CMAKE_INSTALL_BINDIR})

# Replaces: man_MANS=regfsmnt.1 regfschk.1
install(FILES regfsmnt.1 regfschk.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

