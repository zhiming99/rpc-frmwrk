# 1. --- Define Source Files (libcombase_la_SOURCES) ---
set(COMBASE_SOURCES
    clsids.cpp
    autoptr.cpp
    buffer.cpp
    config.cpp
    objbase.cpp
    registry.cpp
    creatins.cpp
    serial.cpp
    dbuserr.cpp
    objfctry.cpp
    dmsgptr.cpp
    seribase.cpp
    variant.cpp
)

# 2. --- Create the Library Target (lib_LTLIBRARIES) ---
# Automatically names the output target 'combase' (generates libcombase.a or libcombase.so)
add_library(combase ${COMBASE_SOURCES})

# 3. --- Define Public Header Install Group (rpcfcombase_HEADERS) ---
# Sets up the destination directory matching $(includedir)/rpcf
set(RPCF_COMBASE_HEADERS
    "../include/autoptr.h"
    "../include/configdb.h"
    "../include/curclsid.h"
    "../include/dbuserr.h"
    "../include/defines.h"
    "../include/dmsgptr.h"
    "../include/nsdef.h"
    "../include/objfctry.h"
    "../include/propids.h"
    "../include/registry.h"
    "../include/rpc.h"
    "../include/seribase.h"
    "../include/stlcont.h"
    "../include/buffer.h"
    "../include/clsids.h"
    "../include/variant.h"
)

# 4. --- Library Target Properties & Versioning (libcombase_la_LDFLAGS) ---
# Replaces Libtool's -version-info flag using standard Unix SOVERSION mechanics.
# It parses the colon-separated VERSION2 string we created (e.g., 0:9:0) into a clean minor version.
string(REPLACE ":" "." SO_VERSION "${VERSION2}")

set_target_properties(combase PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${SO_VERSION}"
    OUTPUT_NAME "combase"
)

find_package(Threads REQUIRED)
# 5. --- Link Dependencies & Target Libraries (libcombase_la_LIBADD) ---
# We link against the global thread pool, dynamic loader, lz4, and our previously found DBus layers.
target_link_libraries(combase PRIVATE
    utils              # Maps to your local 3rd_party compilation target
    Threads::Threads   # Modern, platform-agnostic alternative to -lpthread
    ${CMAKE_DL_LIBS}   # Modern, platform-agnostic alternative to -ldl
    PkgConfig::LZ4     # Our discovered LZ4 package module target from Part 6
    PkgConfig::DBUS    # Our discovered DBus-1 package module target from Part 6
    PkgConfig::JSONCPP # Handled cleanly via targets instead of raw global strings
)

# Explicitly configure search paths for internal 3rd_party binaries if needed
target_link_directories(combase PRIVATE "${CMAKE_BINARY_DIR}/3rd_party")

# 6. --- Installation Rules (libcombase_ladir & rpcfcombasedir) ---
# This replicates your default build tree destination targets perfectly upon running 'make install'
include(GNUInstallDirs)

install(TARGETS combase
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # Installs static libraries (.a)
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # Installs shared objects (.so)
)

install(FILES ${RPCF_COMBASE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpcf
)
