cmake_minimum_required(VERSION 3.18)
#project(smoketest LANGUAGES C CXX)

# Expose standard directory variables (maps bindir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
# Emulates: include_ipcdir=$(top_srcdir)/ipc
set(include_ipcdir "${PROJECT_SOURCE_DIR}/ipc")

# ==============================================================================
# 2. Executable Program Target (smoketest)
# ==============================================================================
# Replaces: smktest_PROGRAMS=smoketest
add_executable(smoketest main.cpp)

target_link_directories(smoketest PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
)
# Replaces: smoketest_CPPFLAGS=$(smktest_CPPFLAGS)
target_include_directories(smoketest PRIVATE ${include_ipcdir})
set_target_properties(smoketest PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: smoketest_LDADD=$(smktest_LDFLAGS)
target_link_libraries(smoketest PRIVATE 
    ${TSTLDFLG} # Maps from @TSTLDFLG@ via your master scope
    combase ipc
)

# ==============================================================================
# 3. Installation Routines
# ==============================================================================
# Replaces: smktestdir=$(bindir)/rpcf
install(TARGETS smoketest DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

