cmake_minimum_required(VERSION 3.18)
#project(rpcf_logger LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir, includedir, mandir)
include(GNUInstallDirs)

message( STATUS "project source dir is ${PROJECT_SOURCE_DIR}" )
# ==============================================================================
# 1. Base Variables and Architecture Macros Mapping
# ==============================================================================
set(include_ipcdir "${CMAKE_SOURCE_DIR}/ipc")

set(OFFSET_MACRO "")
if(BITWIDTH32)
    set(OFFSET_MACRO "-D_FILE_OFFSET_BITS=64")
endif()

# Replicate compilation directory search paths
set(LOGGER_COMMON_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
    "${include_ipcdir}"
)


#add_custom_target(generate_logger_files DEPENDS ${GENERATED_LOGGER_FILES})

# ==============================================================================
# 3. Shared Client Library Target (liblogcli.la)
# ==============================================================================
add_library(logcli SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/LogServicecli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogServicecli.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logclibase.h
)
#add_dependencies(logcli generate_logger_files)

target_include_directories(logcli PRIVATE ${LOGGER_COMMON_INCLUDES})
set_target_properties(logcli PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OFFSET_MACRO)
    target_compile_definitions(logcli PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: liblogcli_la_LDFLAGS= -version-info @VERSION2@
#set_target_properties(logcli PROPERTIES 
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Replaces: liblogcli_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(logcli PRIVATE "${CMAKE_BINARY_DIR}/3rd_party")
target_link_libraries(logcli PRIVATE
    ipc       # Native targets reference matching internal layouts
    combase
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

# ==============================================================================
# 4. Core Daemon Server Target (rpcf_logger)
# ==============================================================================
add_executable(rpcf_logger
    ${CMAKE_CURRENT_SOURCE_DIR}/mainsvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logsvrbase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LogServicesvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogServicesvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logsvrbase.h
)
#add_dependencies(rpcf_logger generate_logger_files)

target_include_directories(rpcf_logger PRIVATE 
    ${LOGGER_COMMON_INCLUDES}
    "${CMAKE_SOURCE_DIR}/monitor/client/cpp/appmancli"
)

set_target_properties(rpcf_logger PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside rpcf_logger_LDFLAGS
)

if(OFFSET_MACRO)
    target_compile_definitions(rpcf_logger PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: rpcf_logger_LDADD
target_link_directories(rpcf_logger PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
    "${CMAKE_BINARY_DIR}/3rd_party"
)
target_link_libraries(rpcf_logger PRIVATE
    ipc
    combase
    appmancli     # Target reference mapping from monitor/client/cpp/appmancli/
    utils
    stdc++
    cppunit
    ${COMMON_LINK}
)

# ==============================================================================
# 5. Command Line Sender Application Target (rpcf_sendlog)
# ==============================================================================
add_executable(rpcf_sendlog ${CMAKE_CURRENT_SOURCE_DIR}/maincli.cpp)
#add_dependencies(rpcf_sendlog generate_logger_files)

target_include_directories(rpcf_sendlog PRIVATE 
    ${LOGGER_COMMON_INCLUDES}
    "${PROJECT_SOURCE_DIR}/monitor/client/cpp/appmancli"
)

set_target_properties(rpcf_sendlog PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside rpcf_sendlog_LDFLAGS
)

if(OFFSET_MACRO)
    target_compile_definitions(rpcf_sendlog PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: rpcf_sendlog_LDADD
target_link_directories(rpcf_sendlog PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
    "${CMAKE_BINARY_DIR}/3rd_party"
)
target_link_libraries(rpcf_sendlog PRIVATE
    logcli        # Local library reference dependency
    ipc
    combase
    utils
    stdc++
    cppunit
    ${COMMON_LINK}
)

# ==============================================================================
# 6. Installation Routines
# ==============================================================================
# Deploy main system executable targets
# Replaces: rpcf_loggerdir=$(bindir) and rpcf_sendlogdir=$(bindir)
install(TARGETS rpcf_logger rpcf_sendlog DESTINATION ${CMAKE_INSTALL_BINDIR})

# Deploy client library binaries
# Replaces: liblogcli_ladir=$(libdir)
install(TARGETS logcli DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Deploy service public API configuration development headers
# Replaces: liblogclidir=$(includedir)/rpcf/ipc
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logclibase.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/ipc
)

# Deploy descriptor assets and system profile targets
# Replaces: rpcf_loggerconfdir=$(sysconfdir)/rpcf / dist_rpcf_loggerconf_DATA
install(FILES 
    loggerdesc.json 
    loggerdrv.json 
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
)

# Deploy UNIX Reference Manual Pages
# Replaces: man_MANS=rpcf_sendlog.1 rpcf_logger.1
install(FILES rpcf_sendlog.1 rpcf_logger.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

