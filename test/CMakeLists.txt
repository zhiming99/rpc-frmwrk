cmake_minimum_required(VERSION 3.18)
project(rpcf_tests LANGUAGES C CXX)

# ==============================================================================
# 1. Base Test Subdirectories (SUBDIRS = iftest btinrt helloworld)
# ==============================================================================
add_subdirectory(iftest)
add_subdirectory(btinrt)
add_subdirectory(helloworld)

# ==============================================================================
# 2. Conditional Test Cases (if ENABLE_TESTCASES)
# ==============================================================================
# Seamlessly hooks into ENABLE_TESTCASES defined in your master CMakeLists.txt
if(ENABLE_TESTCASES)
    add_subdirectory(actcancel)
    add_subdirectory(asynctst)
    add_subdirectory(evtest)
    add_subdirectory(inproc)
    add_subdirectory(katest)
    add_subdirectory(prtest)
    add_subdirectory(sftest)
    add_subdirectory(smoketest)
    add_subdirectory(stmtest)
endif()

