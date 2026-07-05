cmake_minimum_required(VERSION 3.18)
#project(inproc LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libinproc.la)
# ==============================================================================
# Replaces: libinproc_LTLIBRARIES=libinproc.la
add_library(inproc SHARED 
    inprocsvr.cpp 
    inprocli.cpp 
    inprocsvr.h
)

# Replaces: libinproc_la_CPPFLAGS=$(inproc_CPPFLAGS)
target_include_directories(inproc PRIVATE ${include_ipcdir})
set_target_properties(inproc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libinproc_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(inproc PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(inproc PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Target (inproctst)
# ==============================================================================
# Replaces: inproc_PROGRAMS=inproctst
add_executable(inproctst inproctst.cpp inproctst.h)
target_include_directories(inproctst PRIVATE ${include_ipcdir})
set_target_properties(inproctst PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: inproctst_LDADD=$(inproc_LDFLAGS) libinproc.la -lpthread
target_link_libraries(inproctst PRIVATE 
    ${TSTLDFLG} 
    inproc 
    Threads::Threads
)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libinprocdir=$(libdir)/rpcf
install(TARGETS inproc DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: inprocdir=$(bindir)/rpcf
install(TARGETS inproctst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_inproconf_DATA= inproc.json to inproconfdir=$(sysconfdir)/rpcf
install(FILES inproc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

