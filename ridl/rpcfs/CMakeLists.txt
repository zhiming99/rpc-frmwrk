cmake_minimum_required(VERSION 3.18)
#project(genrpcfs LANGUAGES C CXX)

# ==============================================================================
# 1. Define Static Library Target
# ==============================================================================
# Replaces: lib_LIBRARIES=libgenrpcfs.a
# Replaces: libgenrpcfs_a_SOURCES = genpy2.cpp genpy2.h
set_source_files_properties("${BISON_RidlcParser_OUTPUT_HEADER}" PROPERTIES
    GENERATED TRUE
)
add_library(genrpcfs STATIC 
    genpy2.cpp 
    genpy2.h
    "${BISON_RidlcParser_OUTPUT_HEADER}"
)
add_dependencies(genrpcfs generate_ridl_parser)
# ==============================================================================
# 2. Target Include Directories & Compiler Flags
# ==============================================================================
# Replaces: libgenrpcfs_a_CPPFLAGS = -fPIC -I ..
target_include_directories(genrpcfs PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_BINARY_DIR}/.."
    "${CMAKE_parent_BINARY_DIR}"
)

# Enforce Position Independent Code flags natively (-fPIC)
set_target_properties(genrpcfs PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ==============================================================================
# 3. Installation Routines
# ==============================================================================
# Maps the static library layout tracking matching standard GNU installations
include(GNUInstallDirs)
install(TARGETS genrpcfs DESTINATION ${CMAKE_INSTALL_LIBDIR})

