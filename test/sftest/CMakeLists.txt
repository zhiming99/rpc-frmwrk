cmake_minimum_required(VERSION 3.18)
#project(sftest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libsfsvr.la)
# ==============================================================================
# Replaces: libsfsvr_LTLIBRARIES=libsfsvr.la
add_library(sfsvr SHARED 
    sfsvr.cpp 
    sfcli.cpp 
    sfsvr.h
)

target_link_directories(sfsvr PRIVATE ${TSTCFG_LINK_DIR})
target_link_directories(sfsvr PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
)
# Replaces: libsfsvr_la_CPPFLAGS=$(sftest_CPPFLAGS)
target_include_directories(sfsvr PRIVATE ${include_ipcdir})
set_target_properties(sfsvr PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libsfsvr_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(sfsvr PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(sfsvr PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ipc combase
)

# ==============================================================================
# 3. Executable Program Targets (sfclitst & sfsvrtst)
# ==============================================================================

# --- Client Binary ---
add_executable(sfclitst sftest.cpp sftest.h)
target_compile_definitions(sfclitst PRIVATE CLIENT)
target_include_directories(sfclitst PRIVATE ${include_ipcdir})
set_target_properties(sfclitst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(sfclitst PRIVATE ${TSTLDFLG} sfsvr Threads::Threads ipc combase)
target_link_directories(sfclitst PRIVATE ${TSTCFG_LINK_DIR})

# --- Server Binary ---
# Replaces: sfsvrtst_SOURCES= sftest.cpp Readme.txt (omitted text file from compile targets)
add_executable(sfsvrtst sftest.cpp) 
target_compile_definitions(sfsvrtst PRIVATE SERVER)
target_include_directories(sfsvrtst PRIVATE ${include_ipcdir})
set_target_properties(sfsvrtst PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(sfsvrtst PRIVATE ${TSTLDFLG} sfsvr Threads::Threads ipc combase)
target_link_directories(sfsvrtst PRIVATE ${TSTCFG_LINK_DIR})

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libsfsvrdir=$(libdir)/rpcf
install(TARGETS sfsvr DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: sftestdir=$(bindir)/rpcf
install(TARGETS sfclitst sfsvrtst DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_sfclitst_DATA= sfdesc.json to sfclitstdir=$(sysconfdir)/rpcf
install(FILES sfdesc.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

