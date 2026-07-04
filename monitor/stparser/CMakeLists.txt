cmake_minimum_required(VERSION 3.18)
project(rpcf_stparser LANGUAGES C CXX)

# Expose standard directory variables (maps bindir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(combase_dir "${PROJECT_SOURCE_DIR}/combase")

# ==============================================================================
# 2. Bison & Flex Parser Generation Tools
# ==============================================================================
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Emulate bison rule: bison --defines=stparser.h -Wcounterexamples --output=stparser.cpp -r all stparser.y
# Plus the post-generation execution hook: bash updconst.sh
bison_target(StParser stparser.y "${CMAKE_CURRENT_BINARY_DIR}/stparser.cpp"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/stparser.h"
    COMPILE_FLAGS "-Wcounterexamples -r all"
)

# Hook up bash script execution context directly following the Bison processing step
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/stparser.cpp.hook"
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/updconst.sh"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/stparser.cpp.hook"
    DEPENDS ${BISON_StParser_OUTPUTS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Executing custom symbol translation scripts (updconst.sh)"
)

# Emulate flex rule: flex --header-file=stlexer.h -o stlexer.cpp stlexer.l
flex_target(StLexer stlexer.l "${CMAKE_CURRENT_SOURCE_DIR}/stlexer.cpp"
    DEFINES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/stlexer.h"
)

# Bind lexer compilation to parser dependency generation steps
add_flex_bison_dependency(StLexer StParser)

# ==============================================================================
# 3. Define Testing Utility Binary Target (rpcfstc)
# ==============================================================================
set(dep_files
    lvalvar.h 
    nonterm.h 
    parsrctx.cpp 
    parsrctx.h 
    parsrdrv.cpp 
    stast.h 
    stbase.cpp 
    stbase.h 
    stclsids.h 
    stmain.cpp
)

# Consolidate target blueprint files list
set(rpcfstc_SOURCES
    ${BISON_StParser_OUTPUTS}
    ${FLEX_StLexer_OUTPUTS}
    "${CMAKE_CURRENT_BINARY_DIR}/stparser.cpp.hook" # Enforces the script hook sequencing
)

foreach(_file ${dep_files})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
        list(APPEND rpcfstc_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
    else()
        list(APPEND rpcfstc_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${_file}")
    endif()
endforeach()

# Replaces: check_PROGRAMS=rpcfstc
add_executable(rpcfstc ${rpcfstc_SOURCES})

# ==============================================================================
# 4. Target Compile and Link Dependency Configurations
# ==============================================================================
target_include_directories(rpcfstc PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${combase_dir}"
    "${PROJECT_SOURCE_DIR}/include"
)

set_target_properties(rpcfstc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Setup master link maps target hooks resolution 
target_link_directories(rpcfstc PRIVATE "${PROJECT_SOURCE_DIR}/combase")
target_link_libraries(rpcfstc PRIVATE 
    combase # Target reference name from your architecture instead of .la links
)

# ==============================================================================
# 5. Installation Routines
# ==============================================================================
# Replaces: rpcfstcdir=$(bindir)
# Usually test utilities (check_PROGRAMS) are skipped in system deployment blocks,
# but we add this here to stay consistent with the legacy assignment.
install(TARGETS rpcfstc DESTINATION ${CMAKE_INSTALL_BINDIR})

