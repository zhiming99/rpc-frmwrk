cmake_minimum_required(VERSION 3.18)
#project(evtest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libevtst.la)
# ==============================================================================
# Replaces: libevtst_LTLIBRARIES=libevtst.la
add_library(evtst SHARED 
    evtsvr.cpp 
    evtcli.cpp 
    evtsvr.h
)

target_link_directories(evtst PRIVATE ${TSTCFG_LINK_DIR})

# Replaces: libevtst_la_CPPFLAGS=$(evtest_CPPFLAGS)
target_include_directories(evtst PRIVATE ${include_ipcdir})
set_target_properties(evtst PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libevtst_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(evtst PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(evtst PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ipc combase
)

# ==============================================================================
# 3. Executable Program Targets (evtclitst & evtsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(evtclitst evtest.cpp evtest.h)
target_compile_definitions(evtclitst PRIVATE CLIENT)
target_include_directories(evtclitst PRIVATE ${include_ipcdir})
set_target_properties(evtclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(evtclitst PRIVATE ${TSTLDFLG} evtst ipc combase)
target_link_directories(evtclitst PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
add_executable(evtsvrtst evtest.cpp evtest.h)
target_compile_definitions(evtsvrtst PRIVATE SERVER)
target_include_directories(evtsvrtst PRIVATE ${include_ipcdir})
set_target_properties(evtsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(evtsvrtst PRIVATE ${TSTLDFLG} evtst ipc combase)
target_link_directories(evtsvrtst PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libevtstdir=$(libdir)/rpcf
install(TARGETS evtst
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf
    COMPONENT Development
    )

# Replaces: evtestdir=$(bindir)/rpcf
install(TARGETS evtclitst evtsvrtst
    DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf
    COMPONENT Development
    )

# Replaces: dist_evtclitst_DATA= evtdesc.json to evtclitstdir=$(sysconfdir)/rpcf
install(FILES evtdesc.json
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
    COMPONENT Development
    )

