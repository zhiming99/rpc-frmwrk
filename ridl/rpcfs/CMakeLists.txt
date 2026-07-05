cmake_minimum_required(VERSION 3.18)
#project(genrpcfs LANGUAGES C CXX)

# ==============================================================================
# 1. Define Static Library Target
# ==============================================================================
# Replaces: lib_LIBRARIES=libgenrpcfs.a
# Replaces: libgenrpcfs_a_SOURCES = genpy2.cpp genpy2.h
add_library(genrpcfs STATIC 
    genpy2.cpp 
    genpy2.h
)

# ==============================================================================
# 2. Target Include Directories & Compiler Flags
# ==============================================================================
# Replaces: libgenrpcfs_a_CPPFLAGS = -fPIC -I ..
target_include_directories(genrpcfs PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Enforce Position Independent Code flags natively (-fPIC)
set_target_properties(genrpcfs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# 3. Installation Routines
# ==============================================================================
# Maps the static library layout tracking matching standard GNU installations
include(GNUInstallDirs)
install(TARGETS genrpcfs DESTINATION ${CMAKE_INSTALL_LIBDIR})

