cmake_minimum_required(VERSION 3.18)
project(rpcf_oa2check LANGUAGES C CXX)

# Expose standard directory variables (maps sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Static Convenience Archive Target (liboa2check.a)
# ==============================================================================
set(liboa2check_a_SOURCES
    oa2check.cpp
    oa2check.h
    OA2proxycli.h
    OA2proxycli.cpp
)

# Replaces: lib_LIBRARIES=liboa2check.a
# Named 'oa2check' to align natively with parent directory target matching rules
add_library(oa2check STATIC ${liboa2check_a_SOURCES})

# Replaces: liboa2check_a_CPPFLAGS
target_include_directories(oa2check PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/ipc"
    "${PROJECT_SOURCE_DIR}/rpc"
    "${PROJECT_SOURCE_DIR}/combase"
    "${PROJECT_SOURCE_DIR}/3rd_party"
)

# Enforce Position Independent Code flags natively (-fPIC)
set_target_properties(oa2check PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# 2. Installation Routines
# ==============================================================================
# Replaces: oa2check_confdir=$(sysconfdir)/rpcf / dist_oa2check_conf_DATA
install(FILES 
    oa2checkdesc.json 
    oa2check.ridl 
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
)

# Note: liboa2check_adir=./ implies a local convenience static target.
# If your architecture requires distributing this static asset to libdir, 
# un-comment the line below:
# install(TARGETS oa2check DESTINATION ${CMAKE_INSTALL_LIBDIR})

