cmake_minimum_required(VERSION 3.18)
#project(helloworld LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libhellowrd.la)
# ==============================================================================
# Replaces: libhellowrd_LTLIBRARIES=libhellowrd.la
add_library(hellowrd SHARED 
    ifsvr.cpp 
    ifclient.cpp 
    ifsvr.h
)

# Replaces: libhellowrd_la_CPPFLAGS=$(iftest_CPPFLAGS)
target_include_directories(hellowrd PRIVATE ${include_ipcdir})
set_target_properties(hellowrd PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libhellowrd_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(hellowrd PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(hellowrd PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Targets (hwclismk & hwsvrsmk)
# ==============================================================================

# --- Client Binary ---
add_executable(hwclismk iftest.cpp iftest.h)
target_compile_definitions(hwclismk PRIVATE CLIENT)
target_include_directories(hwclismk PRIVATE ${include_ipcdir})
set_target_properties(hwclismk PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(hwclismk PRIVATE ${TSTLDFLG} hellowrd)

# --- Server Binary ---
add_executable(hwsvrsmk iftest.cpp iftest.h)
target_compile_definitions(hwsvrsmk PRIVATE SERVER)
target_include_directories(hwsvrsmk PRIVATE ${include_ipcdir})
set_target_properties(hwsvrsmk PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(hwsvrsmk PRIVATE ${TSTLDFLG} hellowrd)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libhellowrddir=$(libdir)/rpcf
install(TARGETS hellowrd DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: iftestdir=$(bindir)/rpcf
install(TARGETS hwclismk hwsvrsmk DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_hwclismk_DATA= hwdesc.json to hwclismkdir=$(sysconfdir)/rpcf
install(FILES hwdesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

