cmake_minimum_required(VERSION 3.18)
project(rpcf_wsproto LANGUAGES C CXX)

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Static Convenience Archive Target (libwsock.a)
# ==============================================================================
set(libwsock_a_SOURCES
    WebSocket.cpp
    WebSocket.h
    WSLic.txt
    README.md
)

# Replaces: lib_LIBRARIES=libwsock.a
# Note: In the parent directory's Makefile, this library was linked via "-lwsock".
# To maintain complete compatibility with the parent target, we name the target "wsock".
add_library(wsock STATIC ${libwsock_a_SOURCES})

# Enforce Position Independent Code flags natively (-fPIC)
# Replaces: libwsock_a_CPPFLAGS = -fPIC
set_target_properties(wsock PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# 2. Installation Routines
# ==============================================================================
# Replaces: libwsock_adir=./
# Static convenience libraries aren't typically installed globally under Autotools, 
# but if you need to mirror it to your standard lib directory, keep this rule active:
install(TARGETS wsock DESTINATION ${CMAKE_INSTALL_LIBDIR})

