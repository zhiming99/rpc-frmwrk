cmake_minimum_required(VERSION 3.18)
#project(rpcf_java LANGUAGES C CXX)

# Expose standard directory variables (maps bindir, libdir, etc.)
include(GNUInstallDirs)

# Ensure JNI and Java Development Packages are available natively
find_package(JNI REQUIRED COMPONENTS JVM)
find_package(Java REQUIRED COMPONENTS Development)

# ==============================================================================
# 1. Context Configuration & Variables
# ==============================================================================
set(rpcbasedir "${CMAKE_CURRENT_SOURCE_DIR}/src/org/rpcf/rpcbase")
set(jarname "rpcbase-${VERSION2}.jar") # Assumes ${VERSION} comes from your master file
set(javawrapperdir "${CMAKE_INSTALL_FULL_LIBDIR}/rpcf")

# FUSE3 Conditionally Mapped Options
set(fuse3_macro "")
set(fuse3_inc "")
set(fuse3_lib "")
set(swig_fuse_src "")

if(ENABLE_FUSE3)
    set(fuse3_macro "-DFUSE3")
    set(fuse3_inc "${CMAKE_CURRENT_SOURCE_DIR}/../fuse")
    set(fuse3_lib "fuseif") # Refers to native target name instead of .la
    set(swig_fuse_src "fuseif.i")
endif()

# Track original java source structure assets
set(rpcbasejava
    #${CMAKE_CURRENT_SOURCE_DIR}/JavaRpcService.java
    ${rpcbasedir}/JRetVal.java
    ${rpcbasedir}/Helpers.java
    ${rpcbasedir}/JavaRpcContext.java
    ${rpcbasedir}/JavaReqContext.java
    ${rpcbasedir}/RC.java
    ${rpcbasedir}/JavaRpcServer.java
    ${rpcbasedir}/JavaRpcProxy.java
    ${rpcbasedir}/IRpcService.java
)

# ==============================================================================
# 2. SWIG Wrapper Code Generation Target (rpcbase_wrap.cpp)
# ==============================================================================
# Triggers swig -c++ -java ... output generation directly 
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rpcbase_wrap.cpp"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/src/org/rpcf/rpcbase"
    COMMAND swig -c++ -java ${fuse3_macro} 
            -package org.rpcf.rpcbase 
            -outdir "${CMAKE_CURRENT_BINARY_DIR}/src/org/rpcf/rpcbase" 
            -o "${CMAKE_CURRENT_BINARY_DIR}/rpcbase_wrap.cpp" 
            rpcbase.i
    DEPENDS 
        rpcbase.i 
        proxy.i 
        server.i 
        fastrpc.i 
        ${swig_fuse_src}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Java SWIG JNI Interface wrappers"
)

# ==============================================================================
# 3. Native JNI Shared Library Target (librpcbaseJNI.la)
# ==============================================================================
add_library(rpcbaseJNI SHARED "${CMAKE_CURRENT_BINARY_DIR}/rpcbase_wrap.cpp")

message(STATUS "haha,The fuse3 cflags are: ${fuse3_inc}")
# Native Preprocessor compiler configuration definitions and search layouts
target_include_directories(rpcbaseJNI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ipc
    ${CMAKE_SOURCE_DIR}/combase
    ${JNI_INCLUDE_DIRS}
    ${fuse3_inc}
)

if(ENABLE_FUSE3)
    target_compile_definitions(rpcbaseJNI PRIVATE FUSE3)
endif()

#set_target_properties(rpcbaseJNI PROPERTIES 
#    POSITION_INDEPENDENT_CODE ON
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Linkage specifications mapping
find_package(Threads REQUIRED)
target_link_directories(rpcbaseJNI PRIVATE
    ${CMAKE_BINARY_DIR}/combase
    ${CMAKE_BINARY_DIR}/ipc
    ${CMAKE_BINARY_DIR}/3rd_party
)

target_link_libraries(rpcbaseJNI PRIVATE
    ${COMMON_LINK}
    ipc       # Native target tracking names instead of .la assets
    combase
    ${fuse3_lib}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ==============================================================================
# 4. Java Archive Compilation Target (Jar Packaging Steps)
# ==============================================================================
set(JAVA_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/org/rpcf/rpcbase")
file(GLOB OUTPUT_JAVA_SOURCES "${JAVA_OUT_DIR}/*.java")

set(CPP_JAVA_RPC_SERVICE_S_CMD "cpp -E -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServer -DIsServer=\"isServer(){ return true;}\" ${CMAKE_CURRENT_SOURCE_DIR}/JavaRpcService.java -o ${JAVA_OUT_DIR}/JavaRpcServiceS.java")
set(CPP_JAVA_RPC_SERVICE_P_CMD "cpp -E -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxy -DIsServer=\"isServer(){ return false;}\" ${CMAKE_CURRENT_SOURCE_DIR}/JavaRpcService.java -o ${JAVA_OUT_DIR}/JavaRpcServiceP.java")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${jarname}"
    # Replicates: cpp -P -DJavaRpcService=JavaRpcServiceS ...
    COMMAND bash -lc "${CPP_JAVA_RPC_SERVICE_S_CMD}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/JavaRpcService.java"
    # CRUCIAL: VERBATIM forces CMake to escape special shell characters correctly for Bash
    VERBATIM

    COMMAND bash -lc "${CPP_JAVA_RPC_SERVICE_P_CMD}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/JavaRpcService.java"
    # CRUCIAL: VERBATIM forces CMake to escape special shell characters correctly for Bash
    VERBATIM
    
    # Mirror tracked physical assets into building layout workspace
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${rpcbasejava} "${JAVA_OUT_DIR}/"
    
    # Remove conflicting target hooks and compile byteclasses
    COMMAND ${CMAKE_COMMAND} -E rm -f "${JAVA_OUT_DIR}/CJavaProxyImpl.java" "${JAVA_OUT_DIR}/CJavaServerImpl.java"
    COMMAND bash -lc "cd ${JAVA_OUT_DIR}; classpath=./ ${Java_JAVAC_EXECUTABLE} *.java"
    
    # Package Bytecodes to Jar distribution
    COMMAND bash -lc "${Java_JAR_EXECUTABLE} -cf ${CMAKE_CURRENT_BINARY_DIR}/${jarname} org/rpcf/rpcbase/*.class"
    
    # Generate local folder helper development tracking symlink
    COMMAND ${CMAKE_COMMAND} -E create_symlink "${jarname}" "${CMAKE_CURRENT_BINARY_DIR}/rpcbase.jar"
    
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/rpcbase_wrap.cpp" ${rpcbasejava}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src"
    COMMENT "Compiling and packaging Java Class definitions to JAR"
)

# Force JAR evaluation to execute on global target builds
add_custom_target(rpcbase_jar ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${jarname}")
add_dependencies(rpcbase_jar rpcbaseJNI)

# ==============================================================================
# 5. Native Staging & Installation Routines
# ==============================================================================
# Deploy the C++ JNI Wrapper Shared Library object binary
install(TARGETS rpcbaseJNI DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Deploy Java Archive packages
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${jarname}" DESTINATION ${javawrapperdir})

# Replaces: install-data-hook (dynamic symlinking matching setup environment)
install(CODE "
    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        message(STATUS \"Creating symlink: ${javawrapperdir}/rpcbase.jar -> ${jarname}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f \"${javawrapperdir}/rpcbase.jar\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \"${jarname}\" \"${javawrapperdir}/rpcbase.jar\")
    endif()
")

# ==============================================================================
# 6. Uninstallation Configuration Scripts
# ==============================================================================
add_custom_target(uninstall_java
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        if(EXISTS \"${javawrapperdir}/rpcbase.jar\" OR IS_SYMLINK \"${javawrapperdir}/rpcbase.jar\")
            message(STATUS \"Removing symlink: ${javawrapperdir}/rpcbase.jar\")
            execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f \"${javawrapperdir}/rpcbase.jar\")
        endif()
    endif()
")

