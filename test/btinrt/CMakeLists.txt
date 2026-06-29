cmake_minimum_required(VERSION 3.18)
project(btinrt LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Shared Library Target (libbtinrt.la)
# ==============================================================================
# Replaces: libbtinrt_LTLIBRARIES=libbtinrt.la
add_library(btinrt SHARED 
    btinrtsvr.cpp 
    btinrtcli.cpp 
    btinrtsvr.h
)

# Replaces: libbtinrt_la_CPPFLAGS=$(btinrt_CPPFLAGS)
target_include_directories(btinrt PRIVATE ${include_ipcdir})
set_target_properties(btinrt PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libbtinrt_la_LDFLAGS=-version-info @VERSION2@
set_target_properties(btinrt PROPERTIES 
    VERSION "${VERSION2}"
    SOVERSION "${VERSION2}"
)

# Native link dependencies (handles -lpthread and -ldl cleanly across platforms)
find_package(Threads REQUIRED)
target_link_libraries(btinrt PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 3. Executable Program Targets (btinrtcli & btinrtsvr)
# ==============================================================================

# --- Client Binary ---
add_executable(btinrtcli btinrtst.cpp btinrtst.h)
target_compile_definitions(btinrtcli PRIVATE CLIENT)
target_include_directories(btinrtcli PRIVATE ${include_ipcdir})
set_target_properties(btinrtcli PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic
)
target_link_libraries(btinrtcli PRIVATE ${TSTLDFLG} btinrt)

# --- Server Binary ---
add_executable(btinrtsvr btinrtst.cpp btinrtst.h)
target_compile_definitions(btinrtsvr PRIVATE SERVER)
target_include_directories(btinrtsvr PRIVATE ${include_ipcdir})
set_target_properties(btinrtsvr PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic
)
target_link_libraries(btinrtsvr PRIVATE ${TSTLDFLG} btinrt)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================

# Replaces: libbtinrtdir=$(libdir)/rpcf
install(TARGETS btinrt DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf)

# Replaces: btinrtdir=$(bindir)/rpcf
install(TARGETS btinrtcli btinrtsvr DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: dist_btinrtcli_DATA= btinrt.json to btinrtclidir=$(sysconfdir)/rpcf
install(FILES btinrt.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

