cmake_minimum_required(VERSION 3.18)
#project(iftest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libecho.la)
# ==============================================================================
# Replaces: libecho_LTLIBRARIES=libecho.la
add_library(echo SHARED 
    ifsvr.cpp 
    ifclient.cpp 
    ifsvr.h
)

# Replaces: libecho_la_CPPFLAGS=$(iftest_CPPFLAGS)
target_include_directories(echo PRIVATE ${include_ipcdir})
set_target_properties(echo PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libecho_la_LDFLAGS=-version-info @VERSION2@
# Replaces: libecho_la_LIBADD=$(iftest_LDFLAGS) -lpthread -ldl
#set_target_properties(echo PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}" # Adjust if SOVERSION is managed differently in your tree
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(echo PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ipc combase
)

target_link_directories(echo PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 3. Executable Program Targets (ifclismk & ifsvrsmk)
# ==============================================================================

# --- Client Binary ---
add_executable(ifclismk iftest.cpp iftest.h)
target_compile_definitions(ifclismk PRIVATE CLIENT)
target_include_directories(ifclismk PRIVATE ${include_ipcdir})
set_target_properties(ifclismk PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ifclismk PRIVATE ${TSTLDFLG} echo ipc combase)
target_link_directories(ifclismk PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
add_executable(ifsvrsmk iftest.cpp iftest.h)
target_compile_definitions(ifsvrsmk PRIVATE SERVER)
target_include_directories(ifsvrsmk PRIVATE ${include_ipcdir})
set_target_properties(ifsvrsmk PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ifsvrsmk PRIVATE ${TSTLDFLG} echo ipc combase)
target_link_directories(ifsvrsmk PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libechodir=$(libdir)/rpcf
install(TARGETS echo DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: iftestdir=$(bindir)/rpcf
install(TARGETS ifclismk ifsvrsmk DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_ifclismk_DATA= echodesc.json to ifclismkdir=$(sysconfdir)/rpcf
install(FILES echodesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)
