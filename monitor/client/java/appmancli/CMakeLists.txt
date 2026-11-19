cmake_minimum_required(VERSION 3.18)
#project(rpcf_appmancli_java LANGUAGES NONE)

# Expose standard directory variables (maps libdir)
include(GNUInstallDirs)

# Find Java Development Packages natively
find_package(Java REQUIRED COMPONENTS Development)

# ==============================================================================
# 1. Base Variables Mapping (Inherits from Master Scope)
# ==============================================================================
set(javasrcpath "org/rpcf/appmancli")
set(top_dir "${CMAKE_SOURCE_DIR}")
set(ridlfile "${top_dir}/monitor/appmon/appmon.ridl")

set(jarname "appmancli-${VERSION}.jar") # Assumes ${VERSION} comes from your master file
set(shortform "appmancli.jar")
set(jarpkg "dist/${jarname}")
set(appmanclidir "${CMAKE_INSTALL_FULL_LIBDIR}/rpcf")

# Track base and auto-generated java sources
set(basefiles
    ${javasrcpath}/AppManagercli.java
    ${javasrcpath}/MainThread.java
    ${javasrcpath}/AppTimerProxy.java
)

set(autofiles
    ${javasrcpath}/AppManagerclibase.java
    ${javasrcpath}/DeserialArrays.java
    ${javasrcpath}/DeserialMaps.java
    ${javasrcpath}/JavaSerialBase.java
    ${javasrcpath}/JavaSerialHelperP.java
    ${javasrcpath}/JVariant.java
    ${javasrcpath}/KeyValue.java
    ${javasrcpath}/StructFactory.java
)

# ==============================================================================
# 2. Automated Custom Code Generation Target via IDL Compiler (ridlc)
# ==============================================================================
add_custom_command(
    OUTPUT ${autofiles}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${javasrcpath}"
    
    # Stage auxiliary configuration generation assets cleanly inside localized folders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${top_dir}/ridl/mktpl" "${top_dir}/ridl/mktplsvr" "${top_dir}/ridl/pymktpl" "${top_dir}/ridl/mktplcli"
        "${top_dir}/ridl/drvtpl.json" "${top_dir}/ridl/odesctpl2.json" "${top_dir}/ridl/synccfg.py"
        "${top_dir}/ridl/JavaSerialBase.java" "${top_dir}/ridl/JVariant.java" "${top_dir}/ridl/JavaSerialImpl.java"
        "${CMAKE_CURRENT_BINARY_DIR}/"
        
    # Fire compiler execution rules tracking library mapping configurations
    COMMAND ${CMAKE_COMMAND} -E env 
        "LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}/combase:${top_dir}/combase:\$ENV{LD_LIBRARY_PATH}"
        "PYTHONPATH=${CMAKE_BINARY_DIR}/tools:${CMAKE_SOURCE_DIR}/tools"
        $<TARGET_FILE:ridlc> --sync_mode IAppStore=sync 
        --sync_mode IDataProducer=sync 
        --services=AppManager --client --pkgname=org.rpcf.appmancli -jslO . "${ridlfile}"
        
    # Replicates post-generation filtering and file cleanup hooks
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/${javasrcpath}/maincli.java"
    COMMAND ${CMAKE_COMMAND} -E rm -f 
        mktpl pymktpl mktplsvr mktplcli odesctpl2.json drvtpl.json 
        JavaSerialBase.java JVariant.java JavaSerialImpl.java
        *.new appmondesc.json driver.json *.md cmdline synccfg.py
        
    DEPENDS "${ridlfile}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling Java AppMonitor interfaces via ridlc"
)

# Add a logical dependency tracker anchor target to drive serialization hooks
add_custom_target(generate_appman_java_cli DEPENDS ${autofiles})
add_dependencies(generate_appman_java_cli rpcbase_jar ridlc ipc combase)

# ==============================================================================
# 3. Java Compilation & Package JAR Assembly
# ==============================================================================
set(JAR_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${jarpkg}")

# Some 32-bit build containers (e.g. armhf under qemu) run on
# overlayfs, whose directory offsets (d_off) and/or inode numbers do
# not fit a 32-bit readdir()/stat(). javac then dies with 'Value too
# large for defined data type' while walking the source directory.
# The script below compiles in place first and, only on failure,
# stages the sources in JAVA_BUILD_TMPDIR (a tmpfs by default) and
# compiles there. Normal builds never enter the fallback.
set(JAVA_BUILD_TMPDIR "/dev/shm" CACHE PATH
    "Staging directory for the javac fallback on 32-bit/overlayfs builds")

set(APP_JAVAC_SH "${CMAKE_CURRENT_BINARY_DIR}/build_appmancli_jar.sh")
file(WRITE "${APP_JAVAC_SH}" [=[
#!/bin/bash
set -e
SRC="$1"
OUT="$2"
STAGE="$3"
JAVAC="$4"
shift 4
CP="$*"

mkdir -p "$OUT"
if CLASSPATH="$CP" "$JAVAC" -d "$OUT" "$SRC"/*.java; then
    exit 0
fi

echo "javac failed in the build tree; retrying from $STAGE"
TMP=$(mktemp -d "$STAGE/appmancli.XXXXXX" 2>/dev/null || mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp -a "$SRC"/. "$TMP"/

# drop the trailing '.' from the classpath so javac does not
# rescan the (possibly problematic) build tree
CP_RETRY="${CP%:.}"
CLASSPATH="$CP_RETRY" "$JAVAC" "$TMP"/*.java -d "$TMP/dist"

rm -rf "$OUT"
cp -a "$TMP/dist" "$OUT"
]=])

add_custom_command(
    OUTPUT "${JAR_OUTPUT_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/dist"
    
    # Copy static base java files over to the binary workspace to compile alongside autofiles
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp -r ${CMAKE_CURRENT_SOURCE_DIR}/${javasrcpath}/* ${CMAKE_CURRENT_BINARY_DIR}/${javasrcpath}/ || true"
    
    # Replicates: CLASSPATH=... javac ... (with the overlayfs fallback)
    # Assumes project binary rpcbase.jar is located inside ${PROJECT_BINARY_DIR}/java/
    COMMAND bash "${APP_JAVAC_SH}"
            "${CMAKE_CURRENT_BINARY_DIR}/${javasrcpath}"
            "${CMAKE_CURRENT_BINARY_DIR}/dist"
            "${JAVA_BUILD_TMPDIR}"
            "${Java_JAVAC_EXECUTABLE}"
            "${PROJECT_BINARY_DIR}/java/rpcbase.jar:${top_dir}/java/rpcbase.jar:/usr/share/java/commons-cli.jar:."
            
    # Replicates: cd dist && jar cf ...
    COMMAND ${Java_JAR_EXECUTABLE} cf "${JAR_OUTPUT_FILE}" -C "${CMAKE_CURRENT_BINARY_DIR}/dist" .
    
    DEPENDS ${autofiles} ${basefiles}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling Java sources and creating ${jarname}"
)

# Global custom target wrapper to force integration inside default target blocks
add_custom_target(appmancli_jar ALL DEPENDS "${JAR_OUTPUT_FILE}")
add_dependencies(appmancli_jar generate_appman_java_cli )

# ==============================================================================
# 4. Staging & Installation Routines
# ==============================================================================
# Deploy Java Archive packages
install(FILES "${JAR_OUTPUT_FILE}" DESTINATION "${appmanclidir}")

# Replaces: install-data-hook (dynamic symlinking matching setup environment)
install(CODE "
    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        message(STATUS \"Creating symlink: ${appmanclidir}/${shortform} -> ${jarname}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f \"${appmanclidir}/${shortform}\")
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \"${jarname}\" \"${appmanclidir}/${shortform}\")
    endif()
")

# ==============================================================================
# 5. Global Uninstallation Framework (uninstall-local:)
# ==============================================================================
add_custom_target(uninstall_java_appmancli
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" "
    if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        if(EXISTS \"${appmanclidir}/${shortform}\" OR IS_SYMLINK \"${appmanclidir}/${shortform}\")
            message(STATUS \"Removing symlink: ${appmanclidir}/${shortform}\")
            execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f \"${appmanclidir}/${shortform}\")
        endif()
    endif()
")

