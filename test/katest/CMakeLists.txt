cmake_minimum_required(VERSION 3.18)
project(katest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libkasvr.la)
# ==============================================================================
# Replaces: libkasvr_LTLIBRARIES=libkasvr.la
add_library(kasvr SHARED 
    kasvr.cpp 
    kacli.cpp 
    kasvr.h
)

# Replaces: libkasvr_la_CPPFLAGS=$(katest_CPPFLAGS)
target_include_directories(kasvr PRIVATE ${include_ipcdir})
set_target_properties(kasvr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libkasvr_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(kasvr PROPERTIES 
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(kasvr PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Targets (kaclitst & kasvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(kaclitst katest.cpp katest.h)
target_compile_definitions(kaclitst PRIVATE CLIENT)
target_include_directories(kaclitst PRIVATE ${include_ipcdir})
set_target_properties(kaclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kaclitst PRIVATE ${TSTLDFLG} kasvr)

# --- Server Binary ---
add_executable(kasvrtst katest.cpp katest.h)
target_compile_definitions(kasvrtst PRIVATE SERVER)
target_include_directories(kasvrtst PRIVATE ${include_ipcdir})
set_target_properties(kasvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kasvrtst PRIVATE ${TSTLDFLG} kasvr)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libkasvrdir=$(libdir)/rpcf
install(TARGETS kasvr DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: katestdir=$(bindir)/rpcf
install(TARGETS kaclitst kasvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_kaclitst_DATA= kadesc.json to kaclitstdir=$(sysconfdir)/rpcf
install(FILES kadesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

