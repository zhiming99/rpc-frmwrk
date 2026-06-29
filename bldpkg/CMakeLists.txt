cmake_minimum_required(VERSION 3.18)
project(rpcf_bldpkg LANGUAGES NONE)

# Expose standard directory layout variables (maps prefix, bindir, libdir, etc.)
include(GNUInstallDirs)

# ==============================================================================
# 1. Environment Introspection & Package Configuration (From Part 1)
# ==============================================================================
set(rev "0")
set(PACKAGE_NAME "${PROJECT_NAME}") # Inherits from your root configuration name
set(VERSION "${PROJECT_VERSION}")   # Inherits from your root version string

# Staging context variable mirroring Autotools $(DISTDIR)
set(DISTDIR "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}-${VERSION}-${rev}")

if(ENABLE_PYTHON)
    set(ena_python "yes")
else()
    set(ena_python "no")
endif()

if(ENABLE_JS)
    set(ena_js "yes")
else()
    set(ena_js "no")
endif()

if(ENABLE_JAVA)
    set(ena_java "yes")
else()
    set(ena_java "no")
endif()

# ==============================================================================
# 2. Target "debdir" Custom Operation
# ==============================================================================
add_custom_target(debdir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DISTDIR}/debian/${PACKAGE_NAME}"
    
    # Process downstream blddpkg.sh tool tracking environment paths
    # Replicates: arch=$(dpkg --print-architecture); bash blddpkg.sh ...
    COMMAND ${CMAKE_COMMAND} -E env bash -c "
        arch=\$(dpkg --print-architecture)\;
        bash ${CMAKE_CURRENT_SOURCE_DIR}/blddpkg.sh '${DISTDIR}' '\$ENV{DESTDIR}' '${PACKAGE_NAME}_\${VERSION}-${rev}_\${arch}' '${CMAKE_INSTALL_PREFIX}' '${ena_python}' '${ena_js}' '${ena_java}'
    "
    
    # Generate temporary changelog metadata templates
    COMMAND ${CMAKE_COMMAND} -E env bash -c "
        cd '${DISTDIR}' && 
        DEBEMAIL='zhiming <woodhead99@gmail.com>' dch --create -v '${VERSION}-${rev}' --package '${PACKAGE_NAME}' '${PACKAGE_NAME}-${VERSION}-${rev} released'
    "
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Preparing Debian packaging workspace configuration (debdir)"
)

# ==============================================================================
# 3. Target "deb" Custom Operation (From Part 2)
# ==============================================================================
# Triggers full packaging run. Replicates core shell assertions and validations.
add_custom_target(deb_rpcf
    COMMAND ${CMAKE_COMMAND} -E env bash -c "
        if [ -z \"\$ENV{DESTDIR}\" ]\; then
            echo 'Error: DESTDIR environment variable must be explicitly defined when triggering package staging targets.'\;
            exit 1\;
        fi
    "
    # Execute workspace preparations target
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target debdir
    
    # Purge old residual artifacts safely inside binary working directory
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/*.deb" "${CMAKE_CURRENT_BINARY_DIR}/*.ddeb"
    
    # Trigger native debuild routine
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd '${DISTDIR}' && debuild -us -uc -d"
    
    # Replicates complex OS parsing string normalization and tagging rules:
    # Extracted tokens append platform flags like d12 or u24 to binary filenames
    COMMAND ${CMAKE_COMMAND} -E env bash -c "
        debian_ver=\$(cat /etc/os-release | grep VERSION_ID | awk -F '\"' '{print \$2}')\;
        debian_id=\$(cat /etc/os-release | grep '^ID=' | awk -F '=' '{print \$2}')\;
        oid=\${debian_id:0:1}\;
        echo 'debian OS version is '\${debian_ver}\;
        debian_ver=\${oid}\${debian_ver%%.*}\;
        if compgen -G '*.deb' > /dev/null\; then
            for i in *.deb\; do
                mv \"\$i\" \"\${i%%.deb}.\${debian_ver}.deb\"\;
            done\;
        fi\;
        if compgen -G '*.ddeb' > /dev/null\; then
            for i in *.ddeb\; do
                mv \"\$i\" \"\${i%%.ddeb}.\${debian_ver}.ddeb\"\;
            done\;
        fi
    "
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling primary distribution system packages (.deb)"
)

# Handle nested component deb building blocks conditionally matching flags
if(ENABLE_JS)
    add_custom_command(TARGET deb_rpcf POST_BUILD
        COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}/monitor/oinit" --target deb_oinit
        COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${PROJECT_BINARY_DIR}/monitor/oinit/output/python3-oinit_${VERSION}*.deb ${CMAKE_CURRENT_BINARY_DIR}/"
        COMMENT "Consolidating conditional monitor oinit Debian artifacts"
    )
endif()

add_custom_command(TARGET deb_rpcf POST_BUILD
    COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}/monitor/client/python" --target deb_python_appmancli
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${PROJECT_BINARY_DIR}/monitor/client/python/output/python3-appmoncli_${VERSION}*.deb ${CMAKE_CURRENT_BINARY_DIR}/"
    COMMENT "Consolidating client appmoncli Python Debian artifacts"
)

# ==============================================================================
# 4. Target "rpm" Custom Operation (From Part 3)
# ==============================================================================
# Replicates modern automated RedHat distributions compilation chains
add_custom_target(rpm_rpcf
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/rpmbuild"
    COMMAND rpmdev-setuptree
    
    # Leverages your root configuration CPack source targets instead of 'make dist'
    COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR} --target package_source
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${PROJECT_BINARY_DIR}/rpcf-${VERSION}-src.tar.gz \$ENV{HOME}/rpmbuild/SOURCES/${PACKAGE_NAME}-${VERSION}.tar.gz || cp ${PROJECT_BINARY_DIR}/${PACKAGE_NAME}-${VERSION}.tar.gz \$ENV{HOME}/rpmbuild/SOURCES/"
    
    # Fire internal spec custom build script passing host configurations parameters
    # Replaces: host_cpu parameter with CMAKE_SYSTEM_PROCESSOR or platform mapping
    COMMAND ${CMAKE_COMMAND} -E env bash -c "bash ${CMAKE_CURRENT_SOURCE_DIR}/bldrpm.sh '${PROJECT_SOURCE_DIR}/' '${PACKAGE_NAME}' '${CMAKE_SYSTEM_PROCESSOR}'"
    
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling primary distribution RedHat packages (.rpm)"
)

# Handle nested component rpm building blocks conditionally matching flags
if(ENABLE_JS)
    add_custom_command(TARGET rpm_rpcf POST_BUILD
        COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}/monitor/oinit" --target rpm_oinit
        COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${PROJECT_BINARY_DIR}/monitor/oinit/output/python3-oinit-${VERSION}*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
        COMMENT "Consolidating conditional monitor oinit RPM artifacts"
    )
endif()

add_custom_command(TARGET rpm_rpcf POST_BUILD
    COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}/monitor/client/python" --target rpm_python_appmancli
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${PROJECT_BINARY_DIR}/monitor/client/python/output/python3-appmoncli-${VERSION}*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
    COMMENT "Consolidating client appmoncli Python RPM artifacts"
)

