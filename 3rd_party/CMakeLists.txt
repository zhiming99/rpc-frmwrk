# 1. --- Define Source Files (libutils_a_SOURCES) ---
# Note: Header files (.h) and text documents can be safely included in the 
# source array so they map beautifully inside modern IDE workspace configurations.
set(UTILS_SOURCES
    sha1.cpp
    base64.cpp
    sha1.h
    base64.h
    sha1lic.txt
)

# 2. --- Create the Static Library Target (libutils_a_LIBRARIES) ---
# Enforces an explicit STATIC build boundary to match 'libutils.a'
add_library(utils STATIC ${UTILS_SOURCES})

# 3. --- Define Target Compiler Requirements (libutils_a_CPPFLAGS) ---
# Replaces manual compilation string appends like '-fPIC'.
# This ensures that even though utils is a static archive, it can be safely 
# linked straight inside downstream dynamically shared libraries (.so files).
set_target_properties(utils PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 4. --- Define Public Header Install Group (libutils_HEADERS) ---
set(UTILS_HEADERS
    base64.h
    sha1.h
)

# 5. --- Installation Rules (libutils_adir & libutilsdir) ---
# This replicates your default tree targets upon running 'make install'
include(GNUInstallDirs)

# Installs libutils.a into $(libdir)/rpcf
install(TARGETS utils
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/rpcf
)

# Installs header assets into $(includedir)/rpcf/ipc
install(FILES ${UTILS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf/ipc
)

