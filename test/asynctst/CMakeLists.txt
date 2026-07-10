cmake_minimum_required(VERSION 3.18)
#project(asynctst LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libascsvr.la)
# ==============================================================================
# Replaces: libascsvr_LTLIBRARIES=libascsvr.la
add_library(ascsvr SHARED 
    asyncsvr.cpp 
    asyncli.cpp 
    asyncsvr.h
)

# Replaces: libascsvr_la_CPPFLAGS=$(async_CPPFLAGS)
target_include_directories(ascsvr PRIVATE ${include_ipcdir})
set_target_properties(ascsvr PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_directories(ascsvr PRIVATE ${TSTCFG_LINK_DIR})

# Replaces: libascsvr_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(ascsvr PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(ascsvr PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Targets (asynclitst & asyncsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(asynclitst asynctest.cpp asynctest.h)
target_compile_definitions(asynclitst PRIVATE CLIENT)
target_include_directories(asynclitst PRIVATE ${include_ipcdir})
set_target_properties(asynclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(asynclitst PRIVATE ${TSTLDFLG} ascsvr)
target_link_directories(asynclitst PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
add_executable(asyncsvrtst asynctest.cpp asynctest.h)
target_compile_definitions(asyncsvrtst PRIVATE SERVER)
target_include_directories(asyncsvrtst PRIVATE ${include_ipcdir})
set_target_properties(asyncsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(asyncsvrtst PRIVATE ${TSTLDFLG} ascsvr)
target_link_directories(asyncsvrtst PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libascsvrdir=$(libdir)/rpcf
install(TARGETS ascsvr DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: asynctstdir=$(bindir)/rpcf
install(TARGETS asynclitst asyncsvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_asynclitst_DATA= asyndesc.json to asynclitstdir=$(sysconfdir)/rpcf
install(FILES asyndesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

