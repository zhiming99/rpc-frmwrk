cmake_minimum_required(VERSION 3.18)
#project(actcancel LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libactc.la)
# ==============================================================================
# Replaces: libactc_LTLIBRARIES=libactc.la
add_library(actc SHARED 
    actcsvr.cpp 
    actcli.cpp 
    actcsvr.h
)

target_link_directories(actc PRIVATE ${TSTCFG_LINK_DIR})

# Replaces: libactc_la_CPPFLAGS=$(actc_CPPFLAGS)
target_include_directories(actc PRIVATE ${include_ipcdir})

# Replaces: libactc_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(actc PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(actc PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ipc combase
)

# ==============================================================================
# 3. Executable Program Targets (actclitst & actcsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(actclitst actctest.cpp actctest.h)
target_compile_definitions(actclitst PRIVATE CLIENT)
target_include_directories(actclitst PRIVATE ${include_ipcdir})
set_target_properties(actclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(actclitst PRIVATE ${TSTLDFLG} actc ipc combase)
target_link_directories(actclitst PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
add_executable(actcsvrtst actctest.cpp actctest.h)
target_compile_definitions(actcsvrtst PRIVATE SERVER)
target_include_directories(actcsvrtst PRIVATE ${include_ipcdir})
set_target_properties(actcsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(actcsvrtst PRIVATE ${TSTLDFLG} actc ipc combase)
target_link_directories(actcsvrtst PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libactcdir=$(libdir)/rpcf
install(TARGETS actc DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: actctstdir=$(bindir)/rpcf
install(TARGETS actclitst actcsvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_actclitst_DATA= actcdesc.json to actclitstdir=$(sysconfdir)/rpcf
install(FILES actcdesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

