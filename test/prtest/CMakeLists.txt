cmake_minimum_required(VERSION 3.18)
#project(prtest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libprsvr.la)
# ==============================================================================
# Replaces: libprsvr_LTLIBRARIES=libprsvr.la
add_library(prsvr SHARED 
    prsvr.cpp 
    prcli.cpp 
    prsvr.h
)

target_link_directories(prsvr PRIVATE ${TSTCFG_LINK_DIR})
# Replaces: libprsvr_la_CPPFLAGS=$(prtest_CPPFLAGS)
target_include_directories(prsvr PRIVATE ${include_ipcdir})
set_target_properties(prsvr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libprsvr_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(prsvr PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(prsvr PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ipc combase
)

# ==============================================================================
# 3. Executable Program Targets (prclitst & prsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(prclitst prtest.cpp prtest.h)
target_compile_definitions(prclitst PRIVATE CLIENT)
target_include_directories(prclitst PRIVATE ${include_ipcdir})
set_target_properties(prclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(prclitst PRIVATE ${TSTLDFLG} prsvr ipc combase)
target_link_directories(prclitst PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
add_executable(prsvrtst prtest.cpp prtest.h)
target_compile_definitions(prsvrtst PRIVATE SERVER)
target_include_directories(prsvrtst PRIVATE ${include_ipcdir})
set_target_properties(prsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(prsvrtst PRIVATE ${TSTLDFLG} prsvr ipc combase)
target_link_directories(prsvrtst PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libprsvrdir=$(libdir)/rpcf
install(TARGETS prsvr DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: prtestdir=$(bindir)/rpcf
install(TARGETS prclitst prsvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_prclitst_DATA= prdesc.json to prclitstdir=$(sysconfdir)/rpcf
install(FILES prdesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

