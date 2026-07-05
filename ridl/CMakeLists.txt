cmake_minimum_required(VERSION 3.18)
#project(ridl LANGUAGES C CXX)

# Expose standard directory layout variables (maps bindir, sysconfdir, etc.)
include(GNUInstallDirs)

# ==============================================================================
# 1. Subdirectories (SUBDIRS= rpcfs .)
# ==============================================================================
add_subdirectory(rpcfs)

# ==============================================================================
# 2. Bison & Flex Parser Generation Tools
# ==============================================================================
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Emulate bison -r all --defines=ridlc.h --output=ridlc.cpp ridlc.y
bison_target(RidlParser ridlc.y "${CMAKE_CURRENT_BINARY_DIR}/ridlc.cpp"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/ridlc.h"
    COMPILE_FLAGS "-r all"
)

# Emulate flex ridlc.lex (and track outputs)
flex_target(RidlLexer ridlc.lex "${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp"
    DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/lexer.h"
)

# Bind lexer compilation to parser dependency generation
add_flex_bison_dependency(RidlLexer RidlParser)

# ==============================================================================
# 3. Dynamic Template & Configuration File Generation (sed rules)
# ==============================================================================
# Map Autotools directories to CMake GNUInstallDirs standard keys
set(pkgconfigdir "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
set(ridlc_confdir "${CMAKE_INSTALL_FULL_SYSCONFDIR}/rpcf")

# Native generation matching the sed targets (mktpl, mktplsvr, mktplcli)
foreach(tpl_file mktpl mktplsvr mktplcli)
    # Read the original template
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${tpl_file}.in" TPL_CONTENT)
    
    # Emulate sed -e 's,[@]PKG_CONFIG_PATH[@]...
    string(REPLACE "@PKG_CONFIG_PATH@" "${pkgconfigdir}" TPL_CONTENT "${TPL_CONTENT}")
    string(REPLACE "@DIST_LIB_DIR@" "${CMAKE_INSTALL_FULL_LIBDIR}" TPL_CONTENT "${TPL_CONTENT}")
    string(REPLACE "@BITWIDTH@" "${BITWIDTH}" TPL_CONTENT "${TPL_CONTENT}")
    
    # Write processed script to the binary build folder
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${tpl_file}" "${TPL_CONTENT}")
endforeach()

# Emulate synccfg.py generation rules
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/synccfg.py.in" SYNC_CONTENT)
string(REPLACE "@SRCCFG@" "${ridlc_confdir}/echodesc.json" SYNC_CONTENT "${SYNC_CONTENT}")
string(REPLACE "@BINPATH@" "${CMAKE_INSTALL_FULL_BINDIR}/rpcf" SYNC_CONTENT "${SYNC_CONTENT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/synccfg.py" "${SYNC_CONTENT}")

# ==============================================================================
# 4. Define Executable Target (ridlc)
# ==============================================================================
# Source collection matching ridlc_SOURCES and nodist_ridlc_SOURCES
set(ridlc_SOURCES
    gencpp.cpp astnode.cpp main.cpp astnode.h gencpp.h idlclsid.h
    genpy.cpp genpy.h fulltest.ridl tinyif.ridl README.md README_cn.md odesctpl.json
    odesctpl2.json drvtpl.json mktpl.in synccfg.py.in pymktpl genjava.cpp genjava.h
    genfuse.h genfuse.cpp gencpp2.h gencpp2.cpp genfuse2.h genfuse2.cpp genrpcfs.cpp
    genjs.cpp genjs.h mktplsvr.in mktplcli.in
    
    # Generated Built Sources
    ${BISON_RidlParser_OUTPUTS}
    ${FLEX_RidlLexer_OUTPUTS}
)

add_executable(ridlc ${ridlc_SOURCES})

# ==============================================================================
# 5. Compiler & Preprocessor Definitions (ridlc_CPPFLAGS)
# ==============================================================================
# Map Include directories (-I)
target_include_directories(ridlc PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR} # Holds generated ridlc.h / lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/ipc   # Emulates $(top_srcdir)/ipc
    ${PROJECT_SOURCE_DIR}/include
)

# Position Independent Code implementation (-fPIC)
set_target_properties(ridlc PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Conditional Preprocessor Flag Mapping Block
if(ENABLE_FUSE3)
    target_compile_definitions(ridlc PRIVATE FUSE3)
endif()

if(ENABLE_JAVA)
    target_compile_definitions(ridlc PRIVATE JAVA)
endif()

if(ENABLE_PYTHON)
    target_compile_definitions(ridlc PRIVATE PYTHON)
endif()

if(ENABLE_KRB5)
    target_compile_definitions(ridlc PRIVATE KRB5)
endif()

if(ENABLE_AUTH)
    target_compile_definitions(ridlc PRIVATE AUTH)
endif()

if(ENABLE_OPENSSL)
    target_compile_definitions(ridlc PRIVATE OPENSSL)
endif()

if(ENABLE_GMSSL)
    target_compile_definitions(ridlc PRIVATE GMSSL)
endif()

if(ENABLE_JS)
    target_compile_definitions(ridlc PRIVATE JAVASCRIPT)
endif()

# ==============================================================================
# 6. Linker Dependencies (ridlc_LDADD)
# ==============================================================================
# Specify explicit tracking link directories if search paths are missed
target_link_directories(ridlc PRIVATE
    ${CMAKE_BINARY_DIR}/combase
    ${CMAKE_BINARY_DIR}/ipc
    ${CMAKE_BINARY_DIR}/3rd_party
    ${CMAKE_BINARY_DIR}/debug64/
)

# Bind downstream libraries to the target executable framework
target_link_libraries(ridlc PRIVATE
    ${COMMON_LINK}
    combase       # Emulates link to static/shared target library instead of .la
    genrpcfs      # Target from sub-directory: rpcfs/
    utils
    jsoncpp
)

# ==============================================================================
# 7. Installation Routines (ridlcdir & ridlc_conf_DATA)
# ==============================================================================
# Install the ridlc executable to bindir
install(TARGETS ridlc DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install configuration assets and templates to sysconfdir/rpcf
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/odesctpl.json
    ${CMAKE_CURRENT_SOURCE_DIR}/odesctpl2.json
    ${CMAKE_CURRENT_SOURCE_DIR}/drvtpl.json
    ${CMAKE_CURRENT_SOURCE_DIR}/seribase.py
    ${CMAKE_CURRENT_SOURCE_DIR}/pymktpl
    ${CMAKE_CURRENT_SOURCE_DIR}/JavaSerialBase.java
    ${CMAKE_CURRENT_SOURCE_DIR}/JavaSerialImpl.java
    ${CMAKE_CURRENT_SOURCE_DIR}/JVariant.java
    
    # Generated Assets from the Binary Folder
    ${CMAKE_CURRENT_BINARY_DIR}/mktpl
    ${CMAKE_CURRENT_BINARY_DIR}/mktplsvr
    ${CMAKE_CURRENT_BINARY_DIR}/mktplcli
    ${CMAKE_CURRENT_BINARY_DIR}/synccfg.py
    
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
)

# Install Man Pages if found/tracked (man_MANS)
install(FILES ridlc.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ridl.7 DESTINATION ${CMAKE_INSTALL_MANDIR}/man7)

