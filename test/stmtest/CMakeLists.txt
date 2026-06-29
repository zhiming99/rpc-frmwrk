cmake_minimum_required(VERSION 3.18)
project(stmtest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libstmsvr.la)
# ==============================================================================
# Replaces: libstmsvr_LTLIBRARIES=libstmsvr.la
add_library(stmsvr SHARED 
    stmsvr.cpp 
    stmcli.cpp 
    stmsvr.h
)

# Replaces: libstmsvr_la_CPPFLAGS=$(stmtest_CPPFLAGS)
target_include_directories(stmsvr PRIVATE ${include_ipcdir})
set_target_properties(stmsvr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libstmsvr_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(stmsvr PROPERTIES 
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(stmsvr PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Targets (stmclitst & stmsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(stmclitst stmtest.cpp stmtest.h)
target_compile_definitions(stmclitst PRIVATE CLIENT)
target_include_directories(stmclitst PRIVATE ${include_ipcdir})
set_target_properties(stmclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: ext_linkopt=-Wl,-rpath -Wl,$(libdir)/rpcf
# CMake handles RPATH automatically. We add the target installation RPATH explicitly.
set_target_properties(stmclitst PROPERTIES 
    INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/rpcf"
)
target_link_libraries(stmclitst PRIVATE ${TSTLDFLG} stmsvr Threads::Threads)

# --- Server Binary ---
# Replaces: stmsvrtst_SOURCES=stmtest.cpp Readme.txt (omitted text file from compile targets)
add_executable(stmsvrtst stmtest.cpp)
target_compile_definitions(stmsvrtst PRIVATE SERVER)
target_include_directories(stmsvrtst PRIVATE ${include_ipcdir})
set_target_properties(stmsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(stmsvrtst PRIVATE ${TSTLDFLG} stmsvr Threads::Threads)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libstmsvrdir=$(libdir)/rpcf
install(TARGETS stmsvr DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: stmtestdir=$(bindir)/rpcf
install(TARGETS stmclitst stmsvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_stmclitst_DATA= stmdesc.json to stmclitstdir=$(sysconfdir)/rpcf
install(FILES stmdesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

