cmake_minimum_required(VERSION 3.18)
#project(monitor_client LANGUAGES NONE)

# ==============================================================================
# 1. Base Subdirectories (SUBDIRS=cpp/appmancli)
# ==============================================================================
add_subdirectory(cpp/appmancli)

# ==============================================================================
# 2. Conditional Subdirectories
# ==============================================================================

# Hooks into ENABLE_PYTHON from your master CMake configuration
if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()

# Hooks into ENABLE_JAVA from your master CMake configuration
if(ENABLE_JAVA)
    add_subdirectory(java/appmancli)
endif()

# Hooks into ENABLE_JS from your master CMake configuration
if(ENABLE_JS)
    add_subdirectory(js/appmoncli)
endif()


