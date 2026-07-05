# 1. --- Manage Architecture Conditional Core Flags (OFFSET_MACRO Selection) ---
# Replaces if BITWIDTH32 block using the condition variable we cached in Part 19
set(OFFSET_MACRO "")
if(BITWIDTH32)
    set(OFFSET_MACRO "-D_FILE_OFFSET_BITS=64")
endif()

# 2. --- Define Source and Header Assets ---
set(FUSEIF_SOURCES
    fuseif.cpp
    fuseif_ll.cpp
    serijson.cpp
    fuse_i.h # Included directly here to replace local EXTRA_DIST
)

set(FUSEIF_PUBLIC_HEADERS
    fuseif.h
    fusedefs.h
    serijson.h
)

# 3. --- Create and Configure the Interface Library Target (lib_LTLIBRARIES) ---
add_library(fuseif ${FUSEIF_SOURCES})

# Handle compilation include parameters and architecture flag variables
target_include_directories(fuseif PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ipc")
if(NOT "${OFFSET_MACRO}" STREQUAL "")
    target_compile_options(fuseif PRIVATE ${OFFSET_MACRO})
endif()

# Apply version properties mapping matching Libtool parameters
#string(REPLACE ":" "." SO_VERSION "${VERSION2}")
#set_target_properties(fuseif PROPERTIES
#    VERSION "${PROJECT_VERSION}"
#    SOVERSION "${SO_VERSION}"
#    OUTPUT_NAME "fuseif"
#)

# Bind package dependencies using clean target definitions
target_link_libraries(fuseif PRIVATE
    ipc             # Automatically chains down to combase, threads, lz4, and utils!
    PkgConfig::FUSE3 # Our discovered FUSE3 package target from Part 8
)

# 4. --- Compile Host Server Utility (hostsvr_PROGRAMS Execution Layer) ---
add_executable(hostsvr rpcfshst.cpp)

# Target-specific parameters replace global environment injections cleanly
target_include_directories(hostsvr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ipc")
target_compile_options(hostsvr PRIVATE -fPIC ${OFFSET_MACRO})
target_compile_definitions(hostsvr PRIVATE SERVER) # Replaces -DSERVER

target_link_libraries(hostsvr PRIVATE
    combase
    fuseif
    ipc
    PkgConfig::FUSE3
)

# 5. --- Compile Host Client Utility (hostcli_PROGRAMS Execution Layer) ---
# Reuses the exact same source file, separating target definitions safely
add_executable(hostcli rpcfshst.cpp)

target_include_directories(hostcli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ipc")
target_compile_options(hostcli PRIVATE -fPIC ${OFFSET_MACRO})
target_compile_definitions(hostcli PRIVATE CLIENT) # Replaces -DCLIENT

target_link_libraries(hostcli PRIVATE
    combase
    fuseif
    ipc
    PkgConfig::FUSE3
)

# 6. --- Visual Mapping for Python Script Examples (EXTRA_DIST Translation) ---
set(PYTHON_EXAMPLES
    "examples/python/actcancel/ActiveCancelcli.py"   "examples/python/actcancel/ActiveCancelsvr.py"
    "examples/python/actcancel/maincli.py"           "examples/python/evtest/EventTestcli.py"
    "examples/python/evtest/maincli.py"              "examples/python/evtest/mainsvr.py"
    "examples/python/hellowld/HelloWorldSvcsvr.py"   "examples/python/hellowld/maincli.py"
    "examples/python/iftest/IfTestsvr.py"             "examples/python/iftest/maincli.py"
    "examples/python/katest/KeepAlivecli.py"         "examples/python/katest/KeepAlivesvr.py"
    "examples/python/katest/maincli.py"              "examples/python/testypes/dumpStream.py"
    "examples/python/testypes/maincli.py"            "examples/python/testypes/mainsvr.py"
    "examples/python/testypes/sendStream.py"          "examples/python/testypes/tailStream.py"
    "examples/python/testypes2/TestTypesSvccli.py"   "examples/python/testypes2/TestTypesSvcsvr.py"
    "examples/python/testypes2/maincli.py"            "examples/python/testypes2/mainsvr.py"
)
add_custom_target(fuse_python_examples SOURCES ${PYTHON_EXAMPLES})

# 7. --- Installation Rules System ---
include(GNUInstallDirs)

# Install runtime utilities (hostsvr, hostcli) and target libraries
install(TARGETS fuseif hostsvr hostcli
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install system include header resources to $(includedir)/rpcf/ipc
install(FILES ${FUSEIF_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/ipc
)
