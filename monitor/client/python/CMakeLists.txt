cmake_minimum_required(VERSION 3.18)
#project(rpcf_monitor_client_python LANGUAGES NONE)

# ==============================================================================
# 1. Subdirectories
# ==============================================================================
add_subdirectory(appmancli)

# ==============================================================================
# 2. Package Building Custom Targets (Replicates deb: and rpm:)
# ==============================================================================
# Triggers bash build_deb.sh via custom target wrapper
add_custom_target(deb_python_appmoncli
    COMMAND bash build_deb.sh
    COMMAND bash -lc "mv ${CMAKE_CURRENT_SOURCE_DIR}/python3-appmoncli_${VERSION2}.deb ${CMAKE_BINARY_DIR}/bldpkg"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Python Client Debian Package"
)

# Triggers bash build_rpm.sh via custom target wrapper
add_custom_target(rpm_python_appmoncli
    COMMAND bash build_rpm.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Python Client RPM Package"
)


