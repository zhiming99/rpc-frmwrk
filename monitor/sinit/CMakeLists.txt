cmake_minimum_required(VERSION 3.18)
#project(rpcf_sinit LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, mandir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(mon_dir "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ==============================================================================
# 2. Shared Library Target (librpcfenc.la)
# ==============================================================================
# Replaces: lib_LTLIBRARIES=librpcfenc.la
add_library(rpcfenc SHARED 
    encdec.cpp 
    encdecg.cpp 
    encdec.h
)

# Replaces: librpcfenc_la_CPPFLAGS
target_include_directories(rpcfenc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mon_dir}/registry
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/ipc
)
set_target_properties(rpcfenc PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OFFSET_MACRO) # Inherited from master or prior scope configurations
    target_compile_definitions(rpcfenc PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: librpcfenc_la_LDFLAGS=-rdynamic -version-info @VERSION2@
#set_target_properties(rpcfenc PROPERTIES 
#    ENABLE_EXPORTS ON
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Handle dynamic OpenSSL and GMSSL extensions conditionally
if(ENABLE_OPENSSL)
    target_compile_definitions(rpcfenc PRIVATE OPENSSL)
    target_link_libraries(rpcfenc PRIVATE ${COMMON_LINK_SSL})
endif()

if(ENABLE_GMSSL)
    target_compile_definitions(rpcfenc PRIVATE GMSSL)
    target_link_libraries(rpcfenc PRIVATE gmssl)
endif()

# ==============================================================================
# 3. Executable Program Target (sinit)
# ==============================================================================
# Replaces: sinit_PROGRAMS=sinit (sinit.1 omitted from compilation sources)
add_executable(sinit sinit.cpp)

# Replaces: sinit_CPPFLAGS
target_include_directories(sinit PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mon_dir}/registry
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/ipc
)
set_target_properties(sinit PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside sinit_LDFLAGS
)

if(OFFSET_MACRO)
    target_compile_definitions(sinit PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: sinit_LDADD
find_package(Threads REQUIRED)
target_link_directories(sinit PRIVATE
    ${CMAKE_BINARY_DIR}/3rd_party
    ${CMAKE_BINARY_DIR}/combase
    ${CMAKE_BINARY_DIR}/ipc
)
target_link_libraries(sinit PRIVATE
    rpcfenc        # Internal target link resolution instead of .la references
    regfs          # Reference to target from ../registry/ folder
    ipc
    combase
    utils
    stdc++
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Ensures local library wraps build completely prior to final binary compiling
add_dependencies(sinit rpcfenc)

# ==============================================================================
# 4. Installation Routines
# ==============================================================================
# Replaces: lib_LTLIBRARIES install rules
install(TARGETS rpcfenc DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Replaces: sinitdir=$(bindir)/rpcf
install(TARGETS sinit DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: man_MANS=sinit.1
install(FILES sinit.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

