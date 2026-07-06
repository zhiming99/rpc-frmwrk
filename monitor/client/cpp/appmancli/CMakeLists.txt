cmake_minimum_required(VERSION 3.18)
#project(rpcf_appmancli LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, includedir, mandir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(include_dir "${CMAKE_SOURCE_DIR}/ipc")
set(mon_dir "${CMAKE_SOURCE_DIR}/monitor")
set(top_dir "${CMAKE_SOURCE_DIR}")

# ==============================================================================
# 2. Automated Custom Code Generation Target via IDL Compiler (ridlc)
# ==============================================================================
# List generated source outputs to establish compile-time tracking chains
set(GENERATED_OUTPUTS
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStorecli.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IDataProducercli.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IUserManagercli.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/IUserManagercli.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IDataProducercli.h"
    "${CMAKE_CURRENT_BINARY_DIR}/IAppStorecli.h"
    "${CMAKE_CURRENT_BINARY_DIR}/appmon.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/appmon.h"
    "${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/appmonstruct.h"
)

set_source_files_properties(${GENERATED_OUTPUTS} PROPERTIES GENERATED TRUE )
set(RIDLC_OPTIONS "ridlc --sync_mode IAppStore=async_p --sync_mode IUserManager=async_p --services=AppManager,SimpleAuth --client -slO \"${CMAKE_CURRENT_BINARY_DIR}\" ${mon_dir}/appmon/appmon.ridl" )
# Replicates: appmon.ridl target step cleanly inside the binary output workspace
add_custom_command(
    OUTPUT ${GENERATED_OUTPUTS}

    # Stage auxiliary configuration generation assets cleanly inside localized folders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${top_dir}/ridl/mktpl" "${top_dir}/ridl/mktplsvr" "${top_dir}/ridl/mktplcli"
        "${top_dir}/ridl/drvtpl.json" "${top_dir}/ridl/odesctpl2.json" "${top_dir}/ridl/synccfg.py"
        "${CMAKE_CURRENT_BINARY_DIR}/"

    COMMAND bash -lc "${RIDLC_OPTIONS}"
    DEPENDS "${mon_dir}/appmon/appmon.ridl" ridlc
        
    # Purge intermediate boilerplate setup files safely
    COMMAND ${CMAKE_COMMAND} -E rm -f 
        "${CMAKE_CURRENT_BINARY_DIR}/mktpl" "${CMAKE_CURRENT_BINARY_DIR}/mktplsvr" 
        "${CMAKE_CURRENT_BINARY_DIR}/mktplcli" "${CMAKE_CURRENT_BINARY_DIR}/odesctpl2.json" 
        "${CMAKE_CURRENT_BINARY_DIR}/drvtpl.json" "${CMAKE_CURRENT_BINARY_DIR}/synccfg.py"
    DEPENDS "${mon_dir}/appmon/appmon.ridl"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling AppMonitor IDL Interfaces via ridlc"
)

# Add a logical dependency tracker anchor target to drive serialization hooks
add_custom_target(generate_appmon_cli DEPENDS ${GENERATED_OUTPUTS})

# ==============================================================================
# 3. Shared Library Target (libappmancli.la)
# ==============================================================================
# Sources include generated files located within the binary environment
add_library(appmancli SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/maincli.cpp     # Tracked as source in part 1
    ${CMAKE_CURRENT_SOURCE_DIR}/AppManagercli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthcli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppManagercli.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthcli.h
    ${GENERATED_OUTPUTS}
)

# Enforce generation step triggers early during compilation pipelines
add_dependencies(appmancli generate_appmon_cli)

# Target compiler configurations and flags management mapping
target_include_directories(appmancli PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mon_dir}/appmon
    ${mon_dir}/registry
    ${PROJECT_SOURCE_DIR}/include
    ${include_dir}
)

set_target_properties(appmancli PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
)

# Libraries link resolution configuration dependencies hooks mapping
find_package(Threads REQUIRED)
target_link_directories(appmancli PRIVATE "${CMAKE_BINARY_DIR}/3rd_party")
target_link_libraries(appmancli PRIVATE
    regfs     # Target references matching local dependencies instead of .la links
    ipc
    combase
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

# ==============================================================================
# 4. Executable Program Target (apptimer)
# ==============================================================================
# Replaces: apptimer_PROGRAMS=apptimer (apptimer.1 omitted from compilation elements)
add_executable(apptimer apptimer.cpp cronexpr.cpp cronexpr.h)

# Replaces: apptimer_CPPFLAGS
target_include_directories(apptimer PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mon_dir}/appmon
    ${mon_dir}/registry
    ${PROJECT_SOURCE_DIR}/include
    ${include_dir}
)

set_target_properties(apptimer PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
    ENABLE_EXPORTS ON # Replaces: -rdynamic inside apptimer_LDFLAGS
)

if(OFFSET_MACRO)
    target_compile_definitions(apptimer PRIVATE "${OFFSET_MACRO}")
endif()

# Replaces: apptimer_LDADD
target_link_directories(apptimer PRIVATE
    ${CMAKE_BINARY_DIR}/3rd_party
    ${CMAKE_BINARY_DIR}/combase
    ${CMAKE_BINARY_DIR}/ipc
)
target_link_libraries(apptimer PRIVATE
    appmancli
    ipc
    combase
    utils
    stdc++
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${COMMON_LINK}
)

# Establish sequencing to verify local library compiles entirely prior to executable builds
add_dependencies(apptimer appmancli)

# ==============================================================================
# 5. Installation Routines
# ==============================================================================
# Deploy the shared library object binary
install(TARGETS appmancli DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Deploy headers configuration mapping rules
# Replaces: appmancliinc_HEADERS
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleAuthcli.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AppManagercli.h
    ${CMAKE_CURRENT_BINARY_DIR}/IUserManagercli.h
    ${CMAKE_CURRENT_BINARY_DIR}/IDataProducercli.h
    ${CMAKE_CURRENT_BINARY_DIR}/IAppStorecli.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/appmancli
)

# Replaces: apptimerdir=$(bindir)/rpcf
install(TARGETS apptimer DESTINATION ${CMAKE_INSTALL_BINDIR}/rpcf)

# Replaces: man_MANS=apptimer.1
install(FILES apptimer.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

