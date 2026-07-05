cmake_minimum_required(VERSION 3.18)
#project(monitor LANGUAGES NONE)

# ==============================================================================
# 1. Base Subdirectories
# ==============================================================================
add_subdirectory(registry)
add_subdirectory(sinit)
add_subdirectory(usermgmt)
add_subdirectory(client)

# ==============================================================================
# 2. Conditional Subdirectories
# ==============================================================================
# Seamlessly hooks into ENABLE_JS defined in your master CMakeLists.txt
if(ENABLE_JS)
    add_subdirectory(oinit)
endif()

# ==============================================================================
# 3. Remaining Subdirectories
# ==============================================================================
add_subdirectory(appmon)
add_subdirectory(stparser)

