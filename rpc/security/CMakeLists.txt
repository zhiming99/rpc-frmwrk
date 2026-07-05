cmake_minimum_required(VERSION 3.18)
#project(rpcf_security LANGUAGES C CXX)

# Expose standard directory variables (maps libdir, sysconfdir)
include(GNUInstallDirs)

# ==============================================================================
# 1. Conditional Subdirectories Setup (SUBDIRS Mapping)
# ==============================================================================
if(ENABLE_JS)
    add_subdirectory(oa2check)
endif()

# ==============================================================================
# 2. Source and Header Management with Autotools Conditionals
# ==============================================================================
set(rpcfauth_headers
    k5proxy.h
    k5server.h
    kdcfdo.h
    secclsid.h
    secfido.h
    security.h
    saproxy.h
    sacheck.h
)

set(libauth_la_SOURCES
    security.cpp
    dllexp.cpp
    saproxy.cpp
    sacheck.cpp
)

# Handle Kerberos V5 (ENABLE_KRB5) Conditionals
set(krb5_defines "")
if(ENABLE_KRB5)
    list(APPEND krb5_defines "KRB5")
    list(APPEND libauth_la_SOURCES
        kdcfdo.cpp
        secfido.cpp
        k5proxy.cpp
        k5server.cpp
    )
endif()

# Handle OAuth2 (ENABLE_JS) Conditionals
set(oa2_defines "")
set(OA2_LIBS "")
if(ENABLE_JS)
    list(APPEND oa2_defines "OA2")
    list(APPEND rpcfauth_headers oa2proxy.h)
    list(APPEND libauth_la_SOURCES oa2proxy.cpp)
    # Replaces: OA2Lib = -L./oa2check -loa2check
    set(OA2_LIBS oa2check) # Refers to the native CMake target from add_subdirectory
endif()

# Consolidate sources array block with explicit headers tracking
list(APPEND libauth_la_SOURCES ${rpcfauth_headers})

# ==============================================================================
# 3. Shared Library Target (libauth.la)
# ==============================================================================
# Replaces: lib_LTLIBRARIES=libauth.la
add_library(auth SHARED ${libauth_la_SOURCES})

# Replaces: libauth_la_CPPFLAGS
target_include_directories(auth PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/ipc"
    "${CMAKE_SOURCE_DIR}/rpc"
    "${CMAKE_SOURCE_DIR}/combase"
    "${CMAKE_SOURCE_DIR}/3rd_party"
    "${CMAKE_SOURCE_DIR}/monitor/appmon"
    "${CMAKE_SOURCE_DIR}/monitor/client/cpp/appmancli"
    "${CMAKE_SOURCE_DIR}/monitor/sinit"
    "${CMAKE_SOURCE_DIR}/monitor/registry"
)

# Apply dynamic compilation scope macro blocks
if(krb5_defines)
    target_compile_definitions(auth PRIVATE ${krb5_defines})
endif()

if(oa2_defines)
    target_compile_definitions(auth PRIVATE ${oa2_defines})
endif()

set_target_properties(auth PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Replaces: libauth_la_LDFLAGS=-version-info @VERSION2@
#set_target_properties(auth PROPERTIES
#    VERSION "${VERSION2}"
#    SOVERSION "${VERSION2}"
#)

# Replaces: libauth_la_LIBADD
find_package(Threads REQUIRED)
target_link_directories(auth PRIVATE
    "${CMAKE_BINARY_DIR}/combase"
    "${CMAKE_BINARY_DIR}/rpc"
    "${CMAKE_BINARY_DIR}/ipc"
    "${CMAKE_BINARY_DIR}/3rd_party"
)

target_link_libraries(auth PRIVATE
    utils
    ${OA2_LIBS}
    rpc          # Native cross-directory target tokens instead of .la references
    ipc
    combase
    appmancli
    regfs
    rpcfenc
    ${COMMON_LINK_KRB5}
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Establish strict target resolution pipeline if internal subdirectory libs exist
if(ENABLE_JS)
    add_dependencies(auth oa2check)
endif()

# ==============================================================================
# 4. Installation Routines
# ==============================================================================
# Replaces: libauth_ladir=$(libdir)
install(TARGETS auth DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Replaces: dist_libauth_conf_DATA=blist.json to libauth_confdir=$(sysconfdir)/rpcf
install(FILES blist.json DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/rpcf)

