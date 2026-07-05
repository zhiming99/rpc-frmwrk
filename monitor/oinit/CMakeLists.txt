cmake_minimum_required(VERSION 3.18)
#project(rpcf_oinit LANGUAGES NONE)

# Include standard directory layout configurations (maps mandir, libdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Environment Introspection & Packaging Command Variable Mapping
# ==============================================================================
# Replaces: IS_VENV := $(shell python3 ...)
execute_process(
    COMMAND python3 -c "import sys; print('true' if sys.prefix != sys.base_prefix else 'false')"
    OUTPUT_VARIABLE IS_VENV
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Replaces: command := $(if $(filter false,$(IS_VENV)),...)
if(IS_VENV STREQUAL "false")
    set(PYTHON_BUILD_CMD "python3" "-m" "build" "--wheel" "--no-isolation" "--outdir" "dist/" ".")
    set(PIP_INSTALL_OPTS "--force-reinstall" "--user" "--break-system-packages")
else()
    set(PYTHON_BUILD_CMD "pip3" "wheel" "." "--no-deps" "--no-build-isolation" "-w" "dist/")
    set(PIP_INSTALL_OPTS "")
endif()

# ==============================================================================
# 2. Target "wheel" Custom Operation
# ==============================================================================
add_custom_target(wheel
    # Replicates: clean and prepare workspace
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/output/*.whl"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit"
    
    # Replicates: cp *.py rpmpkg/oinit; mv rpmpkg/oinit/setup.py rpmpkg/; ...
    # Assumes setup.py is generated previously or tracked in source tree
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/setup.py "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/oinitdesc.json "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/"
    
    # Run directory globbing shell fallback for multi-file extensions securely
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_CURRENT_SOURCE_DIR}/*.py ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/ && cp ${CMAKE_CURRENT_SOURCE_DIR}/README* ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/ && rm -f ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg/oinit/setup.py"
    
    # Replicates: pushd rpmpkg; $(command); cp dist/*.whl ../output; popd
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd ${CMAKE_CURRENT_BINARY_DIR}/rpmpkg && ${PYTHON_BUILD_CMD} && cp dist/*.whl ${CMAKE_CURRENT_BINARY_DIR}/output/"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/rpmpkg"
    COMMAND ${CMAKE_COMMAND} -E echo "wheel package is generated in 'output' directory."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Compiling Python Wheel packages"
)

# ==============================================================================
# 3. Target "deb" Custom Operation
# ==============================================================================
add_custom_target(deb_oinit
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/debpkg"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian"
    COMMAND ${CMAKE_COMMAND} -E echo "10" > "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/compat"
    COMMAND ${CMAKE_COMMAND} -E echo "usr/lib/rpcf/*.whl" > "${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/python3-oinit.install"
    
    # Synchronize debian scripts layout
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp -r ${CMAKE_CURRENT_SOURCE_DIR}/debian/* ${CMAKE_CURRENT_BINARY_DIR}/debpkg/debian/"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_CURRENT_SOURCE_DIR}/*.py ${CMAKE_CURRENT_SOURCE_DIR}/*.md ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/"
    COMMAND bash -lc "cp ${CMAKE_CURRENT_SOURCE_DIR}/oinitdesc.json ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/"
    COMMAND bash -lc "cp ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.deb ${CMAKE_CURRENT_BINARY_DIR}/debpkg/Makefile" 
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/debpkg/oinit/setup.py ${CMAKE_CURRENT_BINARY_DIR}/debpkg/ || true"
    
    # Execute dpkg-buildpackage pipelines
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd ${CMAKE_CURRENT_BINARY_DIR}/debpkg && DEBEMAIL=\"${PACKAGE_BUGREPORT}\" DEBFULLNAME=\"zhiming\" dch --create -v ${VERSION}-${REV} --package python3-oinit \"python3-oinit-${VERSION}-${REV} released\" && dpkg-buildpackage -us -uc"
    VERBATIM
    
    # Consolidate outputs
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/python3-oinit_${VERSION2}-${REV}_*.deb ${CMAKE_BINARY_DIR}/bldpkg/"
    COMMAND ${CMAKE_COMMAND} -E echo "deb package is generated in 'output' directory."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating Debian packages (.deb)"
)

# ==============================================================================
# 4. Target "rpm" Custom Operation
# ==============================================================================
add_custom_target(rpm_oinit
    DEPENDS wheel
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/oinitrpm"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/BUILD"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/BUILDROOT"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/RPMS"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SOURCES"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SPECS"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/oinitrpm/SRPMS"
    
    # Deploy source wheel to SOURCES root
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cp ${CMAKE_CURRENT_BINARY_DIR}/output/*.whl $ENV{HOME}/oinitrpm/SOURCES/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/oinit.spec "$ENV{HOME}/oinitrpm/SPECS/"
    
    # Run rpmbuild configurations
    COMMAND ${CMAKE_COMMAND} -E env bash -c "cd $ENV{HOME}/oinitrpm && rpmbuild -bb --define \"_topdir \`pwd\`\" SPECS/oinit.spec && cp ./RPMS/noarch/*.rpm ${CMAKE_CURRENT_BINARY_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$ENV{HOME}/oinitrpm"
    
    # Consolidate output targets
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "rm -f ${CMAKE_CURRENT_BINARY_DIR}/output/python3-oinit*rpm || true"
    COMMAND ${CMAKE_COMMAND} -E env bash -c "mv ${CMAKE_CURRENT_BINARY_DIR}/python3-oinit*.rpm ${CMAKE_CURRENT_BINARY_DIR}/output/ || true"
    COMMAND ${CMAKE_COMMAND} -E echo "rpm package is generated in 'output' directory."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating RedHat packaging distributions (.rpm)"
)

# ==============================================================================
# 5. Target "localinst" Custom Operation
# ==============================================================================
add_custom_target(localinst
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target wheel
    COMMAND ${CMAKE_COMMAND} -E env bash -c "pip3 install ${PIP_INSTALL_OPTS} ${CMAKE_CURRENT_BINARY_DIR}/output/*.whl"
    COMMENT "Installing python packages locally via pip3"
)

# ==============================================================================
# 6. Standard Installation Hook Layout Definitions
# ==============================================================================
install(FILES oinit.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

