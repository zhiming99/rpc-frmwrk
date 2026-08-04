cmake_minimum_required(VERSION 3.18)
#project(rpcf_appmon LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, sysconfdir, includedir, mandir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables and Architecture Macros Mapping
# ==============================================================================
set(include_ipcdir "${CMAKE_SOURCE_DIR}/ipc")

set(OFFSET_MACRO "")
if(BITWIDTH32)
    set(OFFSET_MACRO "-D_FILE_OFFSET_BITS=64")
endif()

# Replicate broad compile parameters array block
set(APPMON_BASE_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/monitor/registry"
    "${CMAKE_SOURCE_DIR}/monitor/sinit"
    "${CMAKE_SOURCE_DIR}/include"
    "${include_ipcdir}"
)

# ==============================================================================
# 2. Automated IDL Interface Generation Rule (appmon.ridl custom command)
# ==============================================================================
# List physical file outputs produced by the ridlc tool execution step
set(GENERATED_SERVER_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/appmon.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/appmon.h"
    "${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStoresvr.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStoresvr.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppMonitorsvr.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppMonitorsvr.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IDataProducersvr.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IDataProducersvr.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IUserManagersvr.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IUserManagersvr.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStoreExsvr.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStoreExsvr.h"
)

add_custom_command(
    OUTPUT ${GENERATED_SERVER_FILES}
    # Stage boilerplate code-generation templates securely into the binary directory
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/ridl/mktpl" "${CMAKE_SOURCE_DIR}/ridl/mktplsvr" 
        "${CMAKE_SOURCE_DIR}/ridl/mktplcli" "${CMAKE_SOURCE_DIR}/ridl/drvtpl.json" 
        "${CMAKE_SOURCE_DIR}/ridl/odesctpl2.json" "${CMAKE_SOURCE_DIR}/ridl/synccfg.py"
        "${CMAKE_CURRENT_BINARY_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/AppMonitorsvr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AppMonitorsvr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthsvr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthsvr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AppManagersvr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/AppManagersvr.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mainsvr.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/../client/cpp/appmancli/IAppStorecli.h"
        "${CMAKE_CURRENT_BINARY_DIR}/../client/cpp/appmancli/IDataProducercli.h"
        "${CMAKE_CURRENT_BINARY_DIR}/"

    # Execute internal IDL tool compiler mapping environments
    COMMAND $<TARGET_FILE:ridlc> --server -sO.  "${CMAKE_CURRENT_SOURCE_DIR}/appmon.ridl"

    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/appmon.ridl" 
        "${CMAKE_SOURCE_DIR}/monitor/registry/regfsif.ridl"
        
    # Clean up intermediate tracking configuration logs
    COMMAND ${CMAKE_COMMAND} -E rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json
    
    # Replicates sed replacement script hooks natively
    COMMAND ${CMAKE_COMMAND} -E env bash -c "sed -i 's/EnumMyClsid/EnumAMClsid/' appmon.h"
    
    # Run downstream data transformation updating wrappers
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/upddesc.py
    
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling AppMonitor Server IDL Interfaces via ridlc"
)

add_custom_target(generate_appmon_svr DEPENDS ${GENERATED_SERVER_FILES})

# ==============================================================================
# 3. Static Convenience Archive Target (libappmon.a)
# ==============================================================================
add_library(appmon STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/appmon.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/IAppStoresvr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/IAppMonitorsvr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/IDataProducersvr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/IUserManagersvr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/IAppStoreExsvr.cpp
)

add_dependencies(appmon generate_appmon_svr ridlc)
target_include_directories(appmon PRIVATE ${APPMON_BASE_INCLUDES})
set_target_properties(appmon PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(OFFSET_MACRO)
    target_compile_definitions(appmon PRIVATE "${OFFSET_MACRO}")
endif()

# ==============================================================================
# 4. Core Server Executable Target (appmonsvr)
# ==============================================================================
set(appmonsvr_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mainsvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppMonitorsvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthsvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppManagersvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/appmancli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ptlogger.cpp
)

if(ENABLE_FUSE3)
    list(APPEND appmonsvr_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/monfuse.cpp)
endif()

add_executable(appmonsvr ${appmonsvr_SOURCES})
add_dependencies(appmonsvr generate_appmon_svr ridlc)

target_include_directories(appmonsvr PRIVATE
    "${APPMON_BASE_INCLUDES}"
    )

set_target_properties(appmonsvr PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside appmonsvr_LDFLAGS
)

# Apply preprocessor flags conditions safely
if(OFFSET_MACRO)
    target_compile_definitions(appmonsvr PRIVATE "${OFFSET_MACRO}")
endif()

if(ENABLE_FUSE3)
    target_compile_definitions(appmonsvr PRIVATE FUSE3)
endif()

# Set up link dependency maps
find_package(Threads REQUIRED)
target_link_directories(appmonsvr PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/ipc"
    "${CMAKE_BINARY_DIR}/3rd_party"
)

target_link_libraries(appmonsvr PRIVATE
    appmon           # Local convenience static library target link
    appmancli        # Client target reference from monitor/client/cpp/appmancli/
    regfs            # Registry target reference from monitor/registry/
    ipc              # Core IPC target mapping
    rpcfenc          # Crypto wrapper target reference from monitor/sinit/
    combase          # Core foundation utilities library target
    utils
    stdc++
    cppunit
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(ENABLE_FUSE3)
    target_link_libraries(appmonsvr PRIVATE ${COMMON_LINK_FUSE3})
endif()

# ==============================================================================
# 5. Installation Routines
# ==============================================================================
# Deploy main server executable binary target
install(TARGETS appmonsvr DESTINATION ${CMAKE_INSTALL_BINDIR})

# Deploy public interface development API headers configuration maps
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/appmon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/monconst.h
    ${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/appmancli
)

# Deploy application configuration descriptors and reference source model schemas
install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/appmondesc.json 
    appmon.ridl 
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf
)

# Deploy Manual Reference Pages
install(FILES appmonsvr.1 rpcfctl.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Deploy FUSE3 runtime automation and control operational script frameworks conditionally
if(ENABLE_FUSE3)
    install(PROGRAMS
        initappreg.sh appfuncs.sh addapp.sh addpoint.sh chmodapp.sh getpv.sh 
        listapps.sh rmapp.sh rmlink.sh setpv.sh showpoint.sh addlink.sh 
        chownapp.sh rmattr.sh rmpoint.sh setattr.sh showapp.sh updatekeys.py 
        logtool.py namedlock.py authmech.py rmloglink.sh addloglink.sh cloneapp.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf
    )
    
    install(PROGRAMS rpcfctl DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

