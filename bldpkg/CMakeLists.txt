cmake_minimum_required(VERSION 3.18)
#project(rpcf_bldpkg LANGUAGES NONE)

# Expose standard directory layout variables (maps prefix, bindir, libdir, etc.)
include(GNUInstallDirs)

# ==============================================================================
# 1. Environment Introspection & Package Configuration (From Part 1)
# ==============================================================================

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
set(DEB_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/${DISTDIR}-${REV}")
set(SETUP_DEBMETA "arch=\$(dpkg --print-architecture);\
bash ${CMAKE_CURRENT_BINARY_DIR}/blddpkg.sh '${DEB_BUILD_DIR}' '\${DESTDIR}' '${PACKAGE_NAME}_${VERSION}-${REV}_\${arch}' '${CMAKE_INSTALL_PREFIX}' '${ena_python}' '${ena_js}' '${ena_java}' ")

set(CREATE_CHANGE_LOG "cd '${DEB_BUILD_DIR}' && \
DEBEMAIL='zhiming <woodhead99@gmail.com>' dch --create -v '${VERSION}-${REV}' --package '${PACKAGE_NAME}' '${PACKAGE_NAME}-${VERSION}-${REV} released' ")

add_custom_target(debdir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_BUILD_DIR}/debian/${PACKAGE_NAME}"
    
    # Process downstream blddpkg.sh tool tracking environment paths
    # Replicates: arch=$(dpkg --print-architecture); bash blddpkg.sh ...
    COMMAND bash -lc "${SETUP_DEBMETA}"    
    VERBATIM

    # Generate temporary changelog metadata templates
    COMMAND bash -lc "${CREATE_CHANGE_LOG}"
    VERBATIM

    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Preparing Debian packaging workspace configuration (debdir)"
)

# ==============================================================================
# 3. Target "deb" Custom Operation (From Part 2)
# ==============================================================================
# Triggers full packaging run. Replicates core shell assertions and validations.

cmake_host_system_information(RESULT OS_NAME QUERY DISTRIB_ID)
cmake_host_system_information(RESULT OS_VERSION QUERY DISTRIB_VERSION_ID)
string(SUBSTRING "${OS_NAME}" 0 1 oid)
string(REGEX REPLACE "\\..*" "" MAJOR_VERSION "${OS_VERSION}")
set(debian_ver "${oid}${MAJOR_VERSION}")

set(CHECK_DESTDIR "if [ -z \"\${DESTDIR}\" ]; then echo 'Error: DESTDIR environment variable must be explicitly defined when triggering package staging targets.'; exit 1; fi")
set(COPY_DEBS "echo debian OS version is ${OS_VERSION}; \
if compgen -G '*.deb' > /dev/null; then for i in *.deb; do mv \$i \${i%%.deb}.${debian_ver}.deb; done; fi;\
if compgen -G '*.ddeb' > /dev/null; then for i in *.ddeb; do mv \$i \${i%%.ddeb}.${debian_ver}.ddeb; done; fi ")

add_custom_target(deb_rpcf
    COMMAND bash -lc "${CHECK_DESTDIR}" 
    VERBATIM

    # Execute workspace preparations target
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target debdir
    
    # Purge old residual artifacts safely inside binary working directory
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/*.deb"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/*.ddeb"
    
    # Trigger native debuild routine
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd '${DEB_BUILD_DIR}' && debuild -us -uc -d"
    
    # Replicates complex OS parsing string normalization and tagging rules:
    # Extracted tokens append platform flags like d12 or u24 to binary filenames
    COMMAND bash -lc "${COPY_DEBS}"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling primary distribution system packages (.deb)"
)

# Handle nested component deb building blocks conditionally matching flags
if(ENABLE_JS)
    add_custom_command(TARGET deb_rpcf POST_BUILD
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/monitor/oinit" --target deb_oinit
        COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_BINARY_DIR}/monitor/oinit/output/python3-oinit_${VERSION}*.deb ${CMAKE_CURRENT_BINARY_DIR}/"
        COMMENT "Consolidating conditional monitor oinit Debian artifacts"
    )
endif()

add_custom_command(TARGET deb_rpcf POST_BUILD
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/monitor/client/python" --target deb_python_appmoncli
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_BINARY_DIR}/monitor/client/python/output/python3-appmoncli_${VERSION}*.deb ${CMAKE_CURRENT_BINARY_DIR}/"
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
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package_source
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_BINARY_DIR}/rpcf-${VERSION}-src.tar.gz \$ENV{HOME}/rpmbuild/SOURCES/${PACKAGE_NAME}-${VERSION}.tar.gz || cp ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}-${VERSION}.tar.gz \$ENV{HOME}/rpmbuild/SOURCES/"
    
    # Fire internal spec custom build script passing host configurations parameters
    # Replaces: host_cpu parameter with CMAKE_SYSTEM_PROCESSOR or platform mapping
    COMMAND ${CMAKE_COMMAND} -E env bash -c "bash ${CMAKE_CURRENT_SOURCE_DIR}/bldrpm.sh '${PROJECT_SOURCE_DIR}/' '${PACKAGE_NAME}' '${CMAKE_SYSTEM_PROCESSOR}'"
    
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling primary distribution RedHat packages (.rpm)"
)

# Handle nested component rpm building blocks conditionally matching flags
if(ENABLE_JS)
    add_custom_command(TARGET rpm_rpcf POST_BUILD
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/monitor/oinit" --target rpm_oinit
        COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_BINARY_DIR}/monitor/oinit/output/python3-oinit-${VERSION}*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
        COMMENT "Consolidating conditional monitor oinit RPM artifacts"
    )
endif()

add_custom_command(TARGET rpm_rpcf POST_BUILD
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/monitor/client/python" --target rpm_python_appmancli
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_BINARY_DIR}/monitor/client/python/output/python3-appmoncli-${VERSION}*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
    COMMENT "Consolidating client appmoncli Python RPM artifacts"
)

