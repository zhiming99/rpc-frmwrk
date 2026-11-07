# ============================================================================
#  CMakeLists.txt
#
#  CMake port of the hand-written Makefile in this directory.
#  Builds the ANTLR4 based Structured Text parser `rpcfstc`.
#
#  Usage:
#      cmake -S . -B build
#      cmake --build build
#
#  Editing a *.g4 grammar triggers automatic regeneration of the parser
#  sources on the next build.  To force a regeneration:
#      cmake --build build --target regen && cmake --build build
# ============================================================================
cmake_minimum_required(VERSION 3.20)

project(rpcfstc LANGUAGES CXX)

# ----------------------------------------------------------------------------
#  ANTLR 4.13.2 (locally built) -- override with -DANTLR4_DIR=...
# ----------------------------------------------------------------------------
set(ANTLR4_DIR "/home/zhiming/mywork/antlr4-4.13.2"
    CACHE PATH "ANTLR 4.13.2 installation root")
set(ANTLR4_JAR "${ANTLR4_DIR}/antlr4-tool.jar"
    CACHE FILEPATH "ANTLR tool jar used to generate the parsers")
set(ANTLR4_INCLUDE_DIR
    "${ANTLR4_DIR}/runtime/Cpp/run/usr/local/include/antlr4-runtime"
    CACHE PATH "ANTLR4 C++ runtime include directory")
set(ANTLR4_LIB_DIR "${ANTLR4_DIR}/runtime/Cpp/run/usr/local/lib"
    CACHE PATH "ANTLR4 C++ runtime library directory")

# ----------------------------------------------------------------------------
#  Toolchain: the Makefile used -std=c++17 -Wall -Wno-overloaded-virtual
#             together with -O0 -ggdb, i.e. a plain Debug build.
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
#  Dependencies
# ----------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(RPCF_DEP REQUIRED jsoncpp dbus-1)   # pkg-config --cflags jsoncpp dbus-1

find_package(Java REQUIRED COMPONENTS Runtime)        # to run the ANTLR jar

find_library(ANTLR4_RUNTIME_LIB
    NAMES antlr4-runtime
    PATHS "${ANTLR4_LIB_DIR}"
    NO_DEFAULT_PATH REQUIRED)
find_library(COMBASE_LIB NAMES combase REQUIRED)

# ----------------------------------------------------------------------------
#  Include dirs / defines / options shared by every target
#      -I.  -I../../include  -I<antlr>  -DDEBUG  -Wall -Wno-overloaded-virtual
# ----------------------------------------------------------------------------
set(STPARSER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(stparser_common INTERFACE)
target_include_directories(stparser_common INTERFACE
    "${STPARSER_DIR}"
    "${STPARSER_DIR}/../../include"
    "${ANTLR4_INCLUDE_DIR}"
    ${RPCF_DEP_INCLUDE_DIRS})
target_compile_definitions(stparser_common INTERFACE DEBUG)
target_compile_options(stparser_common INTERFACE -Wall -Wno-overloaded-virtual)
target_link_libraries(stparser_common INTERFACE
    "${ANTLR4_RUNTIME_LIB}"
    ${RPCF_DEP_LIBRARIES})

# ----------------------------------------------------------------------------
#  Generated sources -- pragma_expr grammar  (java ... -o ./pragma_expr)
#  The grammar is generated in-source, exactly like the Makefile does, so the
#  quoted includes such as "pragma_expr/pragma_exprParser.h" keep working.
# ----------------------------------------------------------------------------
set(PRAGMA_EXPR_CPP
    "${STPARSER_DIR}/pragma_expr/pragma_exprLexer.cpp"
    "${STPARSER_DIR}/pragma_expr/pragma_exprParser.cpp"
    "${STPARSER_DIR}/pragma_expr/pragma_exprVisitor.cpp"
    "${STPARSER_DIR}/pragma_expr/pragma_exprBaseVisitor.cpp"
    "${STPARSER_DIR}/pragma_expr/pragma_exprListener.cpp"
    "${STPARSER_DIR}/pragma_expr/pragma_exprBaseListener.cpp")
set(PRAGMA_EXPR_H
    "${STPARSER_DIR}/pragma_expr/pragma_exprLexer.h"
    "${STPARSER_DIR}/pragma_expr/pragma_exprParser.h"
    "${STPARSER_DIR}/pragma_expr/pragma_exprVisitor.h"
    "${STPARSER_DIR}/pragma_expr/pragma_exprBaseVisitor.h"
    "${STPARSER_DIR}/pragma_expr/pragma_exprListener.h"
    "${STPARSER_DIR}/pragma_expr/pragma_exprBaseListener.h")

add_custom_command(
    OUTPUT ${PRAGMA_EXPR_CPP} ${PRAGMA_EXPR_H}
    BYPRODUCTS
        "${STPARSER_DIR}/pragma_expr/pragma_expr.tokens"
        "${STPARSER_DIR}/pragma_expr/pragma_exprLexer.tokens"
        "${STPARSER_DIR}/pragma_expr/pragma_expr.interp"
        "${STPARSER_DIR}/pragma_expr/pragma_exprLexer.interp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${STPARSER_DIR}/pragma_expr"
    COMMAND ${Java_JAVA_EXECUTABLE} -jar "${ANTLR4_JAR}"
            -Dlanguage=Cpp -visitor -listener -o ./pragma_expr pragma_expr.g4
    DEPENDS "${STPARSER_DIR}/pragma_expr.g4"
    WORKING_DIRECTORY "${STPARSER_DIR}"
    COMMENT "Generating pragma_expr parser from pragma_expr.g4"
    VERBATIM)

# ----------------------------------------------------------------------------
#  Generated sources -- stlexer + stparser  (java ... -o .)
#  stparser.g4 declares "options { tokenVocab=stlexer; }", so both grammars
#  must be generated by a single ANTLR invocation.
# ----------------------------------------------------------------------------
set(ANTLR_GEN_CPP
    "${STPARSER_DIR}/stparser.cpp"
    "${STPARSER_DIR}/stparserVisitor.cpp"
    "${STPARSER_DIR}/stparserBaseVisitor.cpp"
    "${STPARSER_DIR}/stparserListener.cpp"
    "${STPARSER_DIR}/stparserBaseListener.cpp"
    "${STPARSER_DIR}/stlexer.cpp")
set(ANTLR_GEN_H
    "${STPARSER_DIR}/stparser.h"
    "${STPARSER_DIR}/stparserVisitor.h"
    "${STPARSER_DIR}/stparserBaseVisitor.h"
    "${STPARSER_DIR}/stparserListener.h"
    "${STPARSER_DIR}/stparserBaseListener.h"
    "${STPARSER_DIR}/stlexer.h")

# The Makefile appends two includes to the freshly generated stparser.h:
#     echo '#include "parse_context.h"' >> stparser.h
#     echo '#include "st_parser_ext.h"' >> stparser.h
set(APPEND_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/append_stparser_includes.cmake")
file(WRITE "${APPEND_SCRIPT}"
"file(APPEND \"${STPARSER_DIR}/stparser.h\" \"#include \\\"parse_context.h\\\"\\n#include \\\"st_parser_ext.h\\\"\\n\")\n")

add_custom_command(
    OUTPUT ${ANTLR_GEN_CPP} ${ANTLR_GEN_H}
    BYPRODUCTS
        "${STPARSER_DIR}/stparser.tokens"
        "${STPARSER_DIR}/stlexer.tokens"
        "${STPARSER_DIR}/stparser.interp"
        "${STPARSER_DIR}/stlexer.interp"
    COMMAND ${Java_JAVA_EXECUTABLE} -jar "${ANTLR4_JAR}"
            -Dlanguage=Cpp -visitor -listener -o . stparser.g4 stlexer.g4
    COMMAND ${CMAKE_COMMAND} -P "${APPEND_SCRIPT}"
    DEPENDS
        "${STPARSER_DIR}/stparser.g4"
        "${STPARSER_DIR}/stlexer.g4"
    WORKING_DIRECTORY "${STPARSER_DIR}"
    COMMENT "Generating stparser/stlexer from grammars"
    VERBATIM)

# ----------------------------------------------------------------------------
#  libpragma.a  (pragma.cpp + the generated pragma_expr sources)
# ----------------------------------------------------------------------------
add_library(pragma STATIC
    pragma.cpp
    ${PRAGMA_EXPR_CPP}
    ${PRAGMA_EXPR_H}
    ${ANTLR_GEN_H})            # dependency only: pragma.cpp includes stparser.h
target_link_libraries(pragma PUBLIC stparser_common)

# ----------------------------------------------------------------------------
#  rpcfstc
# ----------------------------------------------------------------------------
add_executable(rpcfstc
    st_main.cpp
    st_parse_listener.cpp
    st_parser_ext.cpp
    astnodes.cpp
    parse_context.cpp
    stsymtab.cpp
    ${ANTLR_GEN_CPP}
    ${ANTLR_GEN_H})

target_link_libraries(rpcfstc PRIVATE
    pragma                     # -L. -lpragma
    stparser_common
    "${COMBASE_LIB}")          # -lcombase

# -Wl,-rpath,<antlr lib dir>
set_target_properties(rpcfstc PROPERTIES
    BUILD_RPATH   "${ANTLR4_LIB_DIR}"
    INSTALL_RPATH "${ANTLR4_LIB_DIR}")

# ----------------------------------------------------------------------------
#  Extra targets
# ----------------------------------------------------------------------------
# Remove the generated sources so the next build regenerates them from the
# grammars (mirrors the `clean` rule of the original Makefile).
add_custom_target(regen
    COMMAND ${CMAKE_COMMAND} -E rm -f
            ${ANTLR_GEN_CPP} ${ANTLR_GEN_H}
            ${PRAGMA_EXPR_CPP} ${PRAGMA_EXPR_H}
    COMMENT "Generated sources removed -- run the build again to regenerate them"
    VERBATIM)
