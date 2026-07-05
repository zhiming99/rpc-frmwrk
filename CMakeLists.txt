cmake_minimum_required(VERSION 3.15)

# --- Replaces AC_INIT, AC_PROG_CC, AC_PROG_CXX, and AC_LANG([C++]) ---
project(rpcf
    VERSION 0.9.0
    HOMEPAGE_URL "https://github.com/zhiming99/rpc-frmwrk"
    LANGUAGES C CXX
)

# 2. Package Variable Mapping
set(REV "0")
set(DISTDIR "${PROJECT_NAME}-${PROJECT_VERSION}")
set(PKGNAME "${DISTDIR}.tar.gz")

# =====================================================================
# GLOBAL INTERCEPTOR: Automatically version every shared library
# =====================================================================
macro(add_library _target)
    # Forward parameters to create the tracking target
    _add_library(${_target} ${ARGN})

    get_target_property(_target_type ${_target} TYPE)
    if(_target_type STREQUAL "SHARED_LIBRARY")
        set_target_properties(${_target} PROPERTIES
            VERSION "0.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_MINOR}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
        )
    endif()
endmacro()



# --- Replaces AX_CXX_COMPILE_STDCXX(11, ...) ---
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Matches 'noext'

# --- Replaces AC_PROG_LEX and AC_PROG_YACC ---
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# --- Replaces AC_CONFIG_HEADERS([config.h]) ---
# Generates the file out-of-source inside your build directory
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ipc)

# --- Configuration Options (Autotools AC_ARG_ENABLE translations) ---

# Replaces AC_ARG_ENABLE([ssl], ...) from Part 1
option(ENABLE_SSL "Build and install with SSL support" ON)

# Replaces AC_ARG_ENABLE([testcases], ...) from Part 1
option(ENABLE_TESTCASES "Build and install with testcases" ON)

# Replaces AC_ARG_ENABLE([krb5], ...)
option(ENABLE_KRB5 "Build and install with Kerberos authentication" ON)

# Replaces AC_ARG_ENABLE([openssl], ...)
option(ENABLE_OPENSSL "Build and install with OpenSSL" ON)

# Replaces AC_ARG_ENABLE([gmssl], ...)
option(ENABLE_GMSSL "Build and install with GmSSL" ON)

# Replaces AC_ARG_ENABLE([fuse3], ...)
option(ENABLE_FUSE3 "Build and install with fuse3" ON)

# Replaces AC_ARG_ENABLE([java], ...)
option(ENABLE_JAVA "Generation of Java skeleton code" ON)

# Replaces AC_ARG_ENABLE([python], ...)
option(ENABLE_PYTHON "Generation of python skeleton code" ON)

# Replaces AC_ARG_ENABLE([js], ...)
option(ENABLE_JS "Generation of JavaScript skeleton code" ON)

# --- Overriding Cross-Dependency Options ---
# Handles the Part 1 rule where --disable-ssl forced gmssl and openssl off
if(NOT ENABLE_SSL)
    set(ENABLE_GMSSL OFF CACHE BOOL "Disabled by ENABLE_SSL" FORCE)
    set(ENABLE_OPENSSL OFF CACHE BOOL "Disabled by ENABLE_SSL" FORCE)
endif()

# --- Configuration Options (Autotools 'if' toggles) ---
option(ENABLE_FUSE3  "Enable FUSE3 support"  OFF)
option(SIPBUILD      "Enable SIP build"      OFF)
option(SIP4          "Enable SIP4 support"    OFF)
option(ENABLE_PYTHON "Enable Python bindings" OFF)
option(SWIG          "Enable SWIG generation" OFF)
option(ENABLE_JAVA   "Enable Java bindings"   OFF)
option(ENABLE_JS     "Enable JS support"     OFF)

# ==============================================================================
# --- Part 3: Library and Program Verification ---
# ==============================================================================

# --- GmSSL Verification Logic ---
if(ENABLE_GMSSL)
    # Check if the header exists under the /usr/local hint directory
    find_path(GMSSL_INCLUDE_DIR gmssl/sm2.h
        PATHS "/usr/local/include"
        NO_DEFAULT_PATH # Forces it to ONLY check /usr/local as the original script did
    )

    # Check if the static library exists under the /usr/local hint directory
    find_library(GMSSL_LIBRARY NAMES libgmssl.a gmssl
        PATHS "/usr/local/lib"
        NO_DEFAULT_PATH
    )

    # If both files exist, CMake marks GmSSL as found. Otherwise, it disables it.
    if(GMSSL_INCLUDE_DIR AND GMSSL_LIBRARY)
        message(STATUS "Checking for gmssl... yes")
    else()
        message(STATUS "Checking for gmssl... no (Required files missing in /usr/local)")
        set(ENABLE_GMSSL OFF CACHE BOOL "Disabled due to missing files" FORCE)
    endif()
endif()


# --- Python and SIP Verification Logic ---
if(ENABLE_PYTHON)
    # Look for the required executable binaries
    find_program(PYTHON3_EXE python3)
    find_program(SIPBUILD_EXE sip-build)
    find_program(SIP_EXE sip)

    # Replaces: AC_MSG_ERROR if python3 is missing
    if(NOT PYTHON3_EXE)
        message(FATAL_ERROR "******Feature 'python' requires python3, but it was not found. Configure will abort.******")
    endif()

    # Replaces: AC_MSG_ERROR if BOTH sip and sip-build are missing
    if(NOT SIP_EXE AND NOT SIPBUILD_EXE)
        message(FATAL_ERROR "******Feature 'python' requires sip or sip-build, but neither was found. Configure will abort.******")
    endif()

    # Conditional sub-flags matching your original build setup
    if(SIPBUILD_EXE)
        set(SIPBUILD ON)
    else()
        set(SIPBUILD OFF)
    endif()

    if(SIP_EXE)
        set(SIP4 ON)
    else()
        set(SIP4 OFF)
    endif()
endif()

# --- JavaScript Verification Logic ---
if(ENABLE_JS)
    find_program(NPM_EXE npm)
    find_program(NODE_EXE node)

    # Replaces: AS_IF([test "x$npm" = "xno" -a "x$node" = "xno"])
    if(NOT NPM_EXE AND NOT NODE_EXE)
        message(FATAL_ERROR "******Feature 'js' requires npm and node, but they were not found. Configure will abort.******")
    endif()
endif()

# --- Java Verification Logic ---
if(ENABLE_JAVA)
    find_program(JAVAC_EXE javac)
    find_program(SWIG_EXE swig)

    # Replaces: AS_IF([test "x$javac" = "xno" -a "x$swig" = "xno"])
    if(NOT JAVAC_EXE AND NOT SWIG_EXE)
        message(FATAL_ERROR "******Feature 'java' requires Java JDK and swig but they were not detected. Configure will abort.******")
    endif()

    # Proactively configures a SWIG flag if your downstream scripts rely on it
    if(SWIG_EXE)
        set(SWIG ON CACHE BOOL "Enabled by SWIG discovery" FORCE)
    endif()
endif()

#option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries by default")
# ==============================================================================
# --- Part 6: Core Library Dependencies (dbus-1, lz4, jsoncpp) ---
# ==============================================================================
include(FindPkgConfig)

# 1. --- Verify dbus-1 (Mandatory) ---
# Replaces AC_CHECK_LIB([dbus-1], ...)
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
if(DBUS_FOUND)
    # Replaces AC_DEFINE([HAVE_LIBdbus_1], [1])
    set(HAVE_LIBdbus_1 1)
    include_directories(${DBUS_INCLUDE_DIRS})
else()
    # Replaces AC_MSG_ERROR
    message(FATAL_ERROR "****** dbus-1 was not found ******")
endif()

# 2. --- Verify lz4 (Mandatory) ---
# Replaces AC_CHECK_LIB([lz4], ...)
pkg_check_modules(LZ4 REQUIRED IMPORTED_TARGET liblz4)
if(NOT LZ4_FOUND)
    # Fallback in case pkg-config package name is just 'lz4'
    pkg_check_modules(LZ4 REQUIRED IMPORTED_TARGET lz4)
else()
    include_directories(${LZ4_INCLUDE_DIRS})
endif()

if(LZ4_FOUND)
    # Replaces AC_DEFINE([HAVE_LIBlz4], [1])
    set(HAVE_LIBlz4 1)
else()
    message(FATAL_ERROR "****** lz4 was not found ******")
endif()

# 3. --- Verify jsoncpp (Optional/Permissive) ---
# Replaces AC_CHECK_LIB([jsoncpp], ...) which lacked an error fallback
pkg_check_modules(JSONCPP IMPORTED_TARGET jsoncpp)
if(JSONCPP_FOUND)
    # Replaces AC_DEFINE([HAVE_LIBjsoncpp], [1])
    set(HAVE_LIBjsoncpp 1)
    include_directories(${JSONCPP_INCLUDE_DIRS})
endif()

# ==============================================================================
# --- Part 7: Conditional Dependencies (OpenSSL, Kerberos & Fuse3 ) ---
# ==============================================================================

# --- OpenSSL Verification Logic ---
if(ENABLE_OPENSSL)
    # Replaces AC_CHECK_LIB([ssl], ...) & AC_CHECK_HEADERS([openssl/ssl.h])
    # CMake has a built-in module that handles both headers and libraries safely
    find_package(OpenSSL REQUIRED)
    
    if(OPENSSL_FOUND)
        # Replaces AC_DEFINE([HAVE_LIBssl], [1])
        set(HAVE_LIBssl 1)
        # Replaces AC_CHECK_HEADERS implicitly setting HAVE_OPENSSL_SSL_H
        set(HAVE_OPENSSL_SSL_H 1)
        include_directories(${OPENSSL_INCLUDE_DIRS})
    endif()
endif()

# --- Kerberos (krb5) Verification Logic ---
if(ENABLE_KRB5)
    # The most robust cross-platform way to find Kerberos is via pkg-config
    pkg_check_modules(KRB5 mit-krb5-gssapi)
    
    if(NOT KRB5_FOUND)
        # Fallback to direct library searches if pkg-config isn't present
        find_library(KRB5_GSS_LIB NAMES gssapi_krb5)
        find_library(KRB5_CORE_LIB NAMES krb5)
        find_path(KRB5_INCLUDE_DIR NAMES gssapi/gssapi_krb5.h)
        
        if(KRB5_GSS_LIB AND KRB5_CORE_LIB AND KRB5_INCLUDE_DIR)
            set(KRB5_FOUND TRUE)
        endif()
    endif()

    if(KRB5_FOUND)
        # Replaces AC_DEFINE([HAVE_LIBgssapi], [1])
        set(HAVE_LIBgssapi 1)
        # Replaces AC_DEFINE([HAVE_LIBkrb5], [1])
        set(HAVE_LIBkrb5 1)
        # Replaces AC_CHECK_HEADERS([gssapi/gssapi_krb5.h])
        set(HAVE_GSSAPI_GSSAPI_KRB5_H 1)
        include_directories(${KRB5_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "****** Kerberos (krb5/gssapi) was enabled but could not be found ******")
    endif()
endif()

# --- FUSE3 Verification Logic ---
if(ENABLE_FUSE3)
    # Replaces AC_CHECK_LIB([fuse3], [fuse_mount], ...)
    # Query pkg-config directly for fuse3
    pkg_check_modules(FUSE3 REQUIRED IMPORTED_TARGET fuse3>=3.0)

    if(FUSE3_FOUND)
        # Replaces AC_DEFINE([HAVE_LIBfuse3], [1])
        set(HAVE_LIBfuse3 1)
        include_directories(${FUSE3_INCLUDE_DIRS})
    else()
        # Replaces AC_MSG_ERROR([fuse3 was not found])
        message(FATAL_ERROR "****** fuse3 was enabled but could not be found ******")
    endif()
endif()

# ==============================================================================
# --- Part 9: System Headers and Type Definitions Loop ---
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ConfigureChecks)

# ==============================================================================
# --- Part 11: Dynamic Dependency Appends (DEPLIBS_ALL Expansion) ---
# ==============================================================================
# 6. --- Replaces DEPLIBS_ALL='dbus-1 jsoncpp cppunit' ---
# Converts the shell variable into a global CMake variable for use in target setups
set(DEPLIBS_ALL "dbus-1" "jsoncpp" "cppunit" CACHE INTERNAL "Global dependencies list")

# 1. --- Replaces case $enable_openssl -> DEPLIBS_ALL+=' openssl' ---
if(ENABLE_OPENSSL AND OPENSSL_FOUND)
    list(APPEND DEPLIBS_ALL "openssl")
endif()

# 2. --- Replaces case $enable_fuse3 -> DEPLIBS_ALL+=' fuse3' ---
# Note: $ac_cv_lib_fuse3_fuse_mount tracks if the test actually succeeded,
# which matches our FUSE3_FOUND variable from Part 8.
if(ENABLE_FUSE3 AND FUSE3_FOUND)
    list(APPEND DEPLIBS_ALL "fuse3")
endif()

# 3. --- Replaces case $enable_krb5 -> DEPLIBS_ALL+=' mit-krb5' ---
if(ENABLE_KRB5 AND KRB5_FOUND)
    list(APPEND DEPLIBS_ALL "mit-krb5")
endif()

# Update the global internal cache item so downstream folders see the full list
set(DEPLIBS_ALL ${DEPLIBS_ALL} CACHE INTERNAL "Global dependencies list" FORCE)

# ==============================================================================
# --- Part 12: Fixed Dependency Profiles (Subst Allocations) ---
# ==============================================================================

# 1. Define profiles as native CMake lists (useful for internal loops)
set(DEPLIBS_LIST         "dbus-1" "jsoncpp")
set(DEPLIBS_CPPUNIT_LIST "dbus-1" "jsoncpp" "cppunit")
set(DEPLIBS_SSL_LIST     "dbus-1" "jsoncpp" "openssl")
set(DEPLIBS_KRB5_LIST    "dbus-1" "jsoncpp" "mit-krb5" "mit-krb5-gssapi")
set(DEPLIBS_FUSE3_LIST   "dbus-1" "jsoncpp" "fuse3")

# 2. Replicate the Autotools space-separated format for template outputs (.pc files)
string(JOIN " " DEPLIBS         ${DEPLIBS_LIST})
string(JOIN " " DEPLIBS_CPPUNIT ${DEPLIBS_CPPUNIT_LIST})
string(JOIN " " DEPLIBS_SSL     ${DEPLIBS_SSL_LIST})
string(JOIN " " DEPLIBS_KRB5    ${DEPLIBS_KRB5_LIST})
string(JOIN " " DEPLIBS_FUSE3   ${DEPLIBS_FUSE3_LIST})

# We also convert the dynamic DEPLIBS_ALL from Part 11 back into a space-separated string
string(JOIN " " DEPLIBS_ALL     "${DEPLIBS_ALL}")

# 3. Cache these values globally so any lower-level config or template generator can read them
set(DEPLIBS         "${DEPLIBS}"         CACHE INTERNAL "Fixed deps profile")
set(DEPLIBS_CPPUNIT "${DEPLIBS_CPPUNIT}" CACHE INTERNAL "CPPUnit profile")
set(DEPLIBS_SSL     "${DEPLIBS_SSL}"     CACHE INTERNAL "SSL profile")
set(DEPLIBS_KRB5    "${DEPLIBS_KRB5}"    CACHE INTERNAL "KRB5 profile")
set(DEPLIBS_FUSE3   "${DEPLIBS_FUSE3}"   CACHE INTERNAL "FUSE3 profile")
set(DEPLIBS_ALL     "${DEPLIBS_ALL}"     CACHE INTERNAL "All active deps profile" FORCE)


# ==============================================================================
# --- Part 13: Full Case Statement Translation (with_sysroot evaluation) ---
# ==============================================================================

# Check if with_sysroot is empty or set explicitly to "no"
if(NOT with_sysroot OR "${with_sysroot}" STREQUAL "no")

    # --- 13A. Standard Local Configurations (no sysroot) ---
    add_compile_definitions(_USE_LIBEV)
    include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party"
    )

    set(SYSROOT "")
    pkg_check_modules(DEPLIBS REQUIRED ${DEPLIBS})
    set(COMMON_LINK "${DEPLIBS_LDFLAGS}")

    pkg_check_modules(DEPLIBS_CPPUNIT REQUIRED ${DEPLIBS_CPPUNIT})
    set(COMMON_LINK_CPPUNIT "${DEPLIBS_CPPUNIT_LDFLAGS}")
    pkg_check_modules(DEPLIBS_SSL REQUIRED ${DEPLIBS_SSL})
    set(COMMON_LINK_SSL     "${DEPLIBS_SSL_LDFLAGS}")
    pkg_check_modules(DEPLIBS_KRB5 REQUIRED ${DEPLIBS_KRB5})
    set(COMMON_LINK_KRB5    "${DEPLIBS_KRB5_LDFLAGS}")
    pkg_check_modules(DEPLIBS_FUSE3 REQUIRED ${DEPLIBS_FUSE3})
    set(COMMON_LINK_FUSE3   "${DEPLIBS_FUSE3_LDFLAGS}")
    pkg_check_modules(DEPLIBS_ALL REQUIRED ${DEPLIBS_ALL})
    set(CXXFLAGS "${DEPLIBS_ALL_CFLAGS} -D_USE_LIBEV -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/3rd_party")

else()

    # --- 13B. Cross-Compilation Configuration (sysroot active - '*') ---
    set(SYSROOT "${with_sysroot}")
    set(PKG_CONFIG_SYSROOT_DIR "${with_sysroot}")
    set(PKG_CONFIG_PATH "")

    # Replaces: find ${with_sysroot} -type d -name pkgconfig
    # Recursively identifies all directories named "pkgconfig" within your sysroot target
    file(GLOB_RECURSE PKGCONFIG_DIRS LIST_DIRECTORIES true "${with_sysroot}/*/pkgconfig")

    # Replaces: tr "\n" ":"
    # Converts our list of folders into a single colon-separated environment path string
    string(JOIN ":" PKG_CONFIG_LIBDIR ${PKGCONFIG_DIRS})

    # Set up global cross-compile flags for CMake's internal package engine
    set(ENV{PKG_CONFIG_SYSROOT_DIR} "${PKG_CONFIG_SYSROOT_DIR}")
    set(ENV{PKG_CONFIG_LIBDIR}      "${PKG_CONFIG_LIBDIR}")

    # Enforce native compile-time configurations
    add_compile_definitions(_USE_LIBEV)
    include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party"
    )

    # Replicate literal output strings required by downstream Makefile templates
    set(PKG_PRE_STR "PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR}")

    set(CXXFLAGS "${PKG_PRE_STR} pkg-config --cflags ${DEPLIBS_ALL} -D_USE_LIBEV -I\$(top_srcdir)/include -I\$(top_srcdir)/3rd_party")
    set(COMMON_LINK         "`${PKG_PRE_STR} pkg-config --libs ${DEPLIBS}`")
    set(COMMON_LINK_CPPUNIT "`${PKG_PRE_STR} pkg-config --libs ${DEPLIBS_CPPUNIT}`")
    set(COMMON_LINK_SSL     "`${PKG_PRE_STR} pkg-config --libs ${DEPLIBS_SSL}`")
    set(COMMON_LINK_KRB5    "`${PKG_PRE_STR} pkg-config --libs ${DEPLIBS_KRB5}`")
    set(COMMON_LINK_FUSE3   "`${PKG_PRE_STR} pkg-config --libs ${DEPLIBS_FUSE3}`")

endif()

# --- 13C. Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(SYSROOT             "${SYSROOT}"             CACHE INTERNAL "")
set(PKG_CONFIG_SYSROOT_DIR "${PKG_CONFIG_SYSROOT_DIR}" CACHE INTERNAL "")
set(PKG_CONFIG_LIBDIR   "${PKG_CONFIG_LIBDIR}"   CACHE INTERNAL "")
set(PKG_CONFIG_PATH     "${PKG_CONFIG_PATH}"     CACHE INTERNAL "")
set(CXXFLAGS            "${CXXFLAGS}"            CACHE INTERNAL "")
set(COMMON_LINK         "${COMMON_LINK}"         CACHE INTERNAL "")
set(COMMON_LINK_CPPUNIT "${COMMON_LINK_CPPUNIT}" CACHE INTERNAL "")
set(COMMON_LINK_SSL     "${COMMON_LINK_SSL}"     CACHE INTERNAL "")
set(COMMON_LINK_KRB5    "${COMMON_LINK_KRB5}"    CACHE INTERNAL "")
set(COMMON_LINK_FUSE3   "${COMMON_LINK_FUSE3}"   CACHE INTERNAL "")

# Replaces: AC_MSG_NOTICE([the cxxflags string is ${CXXFLAGS}])
message(STATUS "the cxxflags string is ${CXXFLAGS}")

# ==============================================================================
# --- Part 15: Architecture Detection (ARM / AArch64 Mappings) ---
# ==============================================================================

# Replaces: case $host in arm* | aarch64)
# We use MATCHES with a regular expression to handle both arm* and aarch64 variants smoothly.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")

    # 1. Inject the -DARM macro globally for compilation
    add_compile_definitions(ARM)

    # Update the tracking string variant if needed by downstream templates
    set(CXXFLAGS "${CXXFLAGS} -DARM")

    # 2. Append -latomic to our raw string link profiles
    set(COMMON_LINK         "${COMMON_LINK} -latomic")
    set(COMMON_LINK_CPPUNIT "${COMMON_LINK_CPPUNIT} -latomic")
    set(COMMON_LINK_SSL     "${COMMON_LINK_SSL} -latomic")
    set(COMMON_LINK_KRB5    "${COMMON_LINK_KRB5} -latomic")
    set(COMMON_LINK_FUSE3   "${COMMON_LINK_FUSE3} -latomic")

    # 3. Force-update the global cache registration values so all files see the modifications
    set(CXXFLAGS            "${CXXFLAGS}"            CACHE INTERNAL "" FORCE)
    set(COMMON_LINK         "${COMMON_LINK}"         CACHE INTERNAL "" FORCE)
    set(COMMON_LINK_CPPUNIT "${COMMON_LINK_CPPUNIT}" CACHE INTERNAL "" FORCE)
    set(COMMON_LINK_SSL     "${COMMON_LINK_SSL}"     CACHE INTERNAL "" FORCE)
    set(COMMON_LINK_KRB5    "${COMMON_LINK_KRB5}"    CACHE INTERNAL "" FORCE)
    set(COMMON_LINK_FUSE3   "${COMMON_LINK_FUSE3}"   CACHE INTERNAL "" FORCE)

endif()

# ==============================================================================
# --- Part 16: Bit-Width Detection & Target Optimization ---
# ==============================================================================

# 1. Replaces the $host case matching for bit-width & arch-specific flags
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)")
    set(BITWIDTH 64)
    add_compile_definitions(BUILD_64=1)

    # Replaces CXXFLAGS+=' -m64'
    add_compile_options(-m64)
    set(CXXFLAGS "${CXXFLAGS} -DBUILD_64=1 -m64")

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(BITWIDTH 64)
    add_compile_definitions(BUILD_64=1)
    set(CXXFLAGS "${CXXFLAGS} -DBUILD_64=1")

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(BITWIDTH 32)
    add_compile_definitions(BUILD_64=0)
    set(CXXFLAGS "${CXXFLAGS} -DBUILD_64=0")

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86")
    set(BITWIDTH 32)
    add_compile_definitions(BUILD_64=0)

    # Replaces CXXFLAGS+=' -m32'
    add_compile_options(-m32)
    set(CXXFLAGS "${CXXFLAGS} -DBUILD_64=0 -m32")
endif()

# 2. Replaces CXXFLAGS+=' -std=c++11'
# Handled safely by CMAKE_CXX_STANDARD 11 at the top of the file, but we append
# it to the string variable for literal backward-compatibility with templates.
set(CXXFLAGS "${CXXFLAGS} -std=c++11")

# 3. Replaces CXXFLAGS+=' -DVERSION_STRING="..."'
# Pass your project's version macro directly into the build pipeline safely escaped.
add_compile_definitions(VERSION_STRING="${PROJECT_VERSION}")
set(CXXFLAGS "${CXXFLAGS} -DVERSION_STRING=\\\"\"${PROJECT_VERSION}\"\\\"")

# 4. Expose the finalized variable updates to the global cache
set(BITWIDTH "${BITWIDTH}" CACHE INTERNAL "Target platform bitwidth size")
set(CXXFLAGS "${CXXFLAGS}" CACHE INTERNAL "" FORCE)

# ==============================================================================
# --- Part 17: Platform Macros & Legacy Linkage Profiles ---
# ==============================================================================

# 1. --- Replaces AC_DEFINE(_GNU_SOURCE, 1, ...) ---
# Automatically enables non-portable GNU extensions across all source compilation units
add_compile_definitions(_GNU_SOURCE=1)

# 2. --- Replaces AC_SUBST( TSTLDFLG, '...' ) ---
# Translate Autotools internal .la library references to their CMake static/shared target locations.
# Note: Instead of using relative file paths like -L, we map them directly to our internal build layout locations.
set(TSTLDFLG_LIST
    "-lipc -lcombase"
    ${COMMON_LINK_CPPUNIT} # Expands your external dependencies natively
)

# Convert our list array into a standard space-separated string required by your downstream .pc templates
string(JOIN " " TSTLDFLG ${TSTLDFLG_LIST})

# 3. --- Replaces PKG_PROG_PKG_CONFIG & PKG_INSTALLDIR ---
# Native CMake initialization hooks for managing and outputting .pc layout configurations.
include(FindPkgConfig)
include(GNUInstallDirs)

# 4. Global Cache Variable Registration (Replaces AC_SUBST mappings)
set(BITWIDTH "${BITWIDTH}" CACHE INTERNAL "" FORCE)
set(TSTLDFLG "${TSTLDFLG}" CACHE INTERNAL "Legacy test linkage template profile")

# ==============================================================================
# --- Part 19: Conditionals Validation & String Substitutions ---
# ==============================================================================

# 1. --- Replaces AM_CONDITIONAL Mappings ---
# These variables validate whether a feature is both enabled AND its dependencies are met.
# They will be used directly in your subdirectory if() statements.

set(ENABLE_AUTH OFF)
if(ENABLE_KRB5 OR ENABLE_JS)
    set(ENABLE_AUTH ON)
endif()

# Replaces: AM_CONDITIONAL([ENABLE_FUSE3], [test ... -a ...])
# Checks both the option selection and whether the library search from Part 8 passed.
if(ENABLE_FUSE3 AND FUSE3_FOUND)
    set(ENABLE_FUSE3_VALID ON)
else()
    set(ENABLE_FUSE3_VALID OFF)
endif()

# Replaces: AM_CONDITIONAL([ENABLE_JAVA], [test "x$javac" = "xyes" ...])
if(ENABLE_JAVA AND JAVAC_EXE)
    set(ENABLE_JAVA_VALID ON)
else()
    set(ENABLE_JAVA_VALID OFF)
endif()

# Replaces: AM_CONDITIONAL([ENABLE_PYTHON], ...)
if(ENABLE_PYTHON AND (SIP4 OR SIPBUILD))
    set(ENABLE_PYTHON_VALID ON)
else()
    set(ENABLE_PYTHON_VALID OFF)
endif()

# Replaces: AM_CONDITIONAL([BITWIDTH32], ...)
if("${BITWIDTH}" EQUAL 32)
    set(BITWIDTH32 ON)
else()
    set(BITWIDTH32 OFF)
endif()

# Replaces: AM_CONDITIONAL([ENABLE_JS], [test "x$npm" = "xyes" ...])
if(ENABLE_JS AND NPM_EXE)
    set(ENABLE_JS_VALID ON)
else()
    set(ENABLE_JS_VALID OFF)
endif()


# 2. --- Replaces VERSION2='$(subst .,:,$(VERSION))' ---
# Replicates the Autotools string manipulation replacing periods with colons (e.g., 0.9.0 -> 0:9:0)
#string(REPLACE "." "." VERSION2 "${PROJECT_VERSION}")


# 3. --- Initializes FEATURES Variable ---
# Ready to track your component strings in subsequent blocks
set(FEATURES "")

# 4. --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(ENABLE_AUTH   "${ENABLE_AUTH}"   CACHE INTERNAL "")
set(BITWIDTH32     "${BITWIDTH32}"     CACHE INTERNAL "")
set(VERSION2       "${PROJECT_VERSION}"       CACHE INTERNAL "Version number")
set(FEATURES       "${FEATURES}"       CACHE INTERNAL "" FORCE)



# ==============================================================================
# --- Part 20: Feature Appends and Package File Lists (GmSSL / OpenSSL) ---
# ==============================================================================

set(ENABLE_WEBSOCK OFF)

# 1. --- GmSSL Flag Processing Loop ---
if(ENABLE_GMSSL)
    set(GMSSL_DEBFILES "usr/bin/rpcf/gmsslkey.sh")
    set(GMSSL_RPMFILES "%{_bindir}/rpcf/gmsslkey.shZZZZZ%{_libdir}/libgmsslpt.so*")
    set(GMSSL_CFGOPT   "")
    set(ENABLE_WEBSOCK ON)
    string(APPEND FEATURES "+gmssl ")
else()
    set(GMSSL_DEBFILES "")
    set(GMSSL_RPMFILES "")
    set(GMSSL_CFGOPT   "--disable-gmssl")
    string(APPEND FEATURES "-gmssl ")
endif()

# 2. --- OpenSSL Flag Processing Loop ---
if(ENABLE_OPENSSL)
    set(OPENSSL_DEBFILES "usr/bin/rpcf/opensslkey.shZZZZZetc/rpcf/openssl.cnf")
    set(OPENSSL_RPMFILES "%{_bindir}/rpcf/opensslkey.shZZZZZ%{_libdir}/libsslpt.so*ZZZZZ%{_sysconfdir}/rpcf/openssl.cnf")
    set(OPENSSL_CFGOPT   "")
    set(OPENSSL_DEBLIBS  "libssl-dev,")
    set(OPENSSL_DEBPKGS  "openssl, expect,")
    set(OPENSSL_RPMLIBS  "openssl-devel,")
    set(OPENSSL_RPMPKGS  "openssl expect")
    set(ENABLE_WEBSOCK   ON)
    string(APPEND FEATURES "+openssl ")
else()
    set(OPENSSL_DEBFILES "")
    set(OPENSSL_RPMFILES "")
    set(OPENSSL_CFGOPT   "--disable-openssl")
    set(OPENSSL_DEBLIBS  "")
    set(OPENSSL_DEBPKGS  "")
    set(OPENSSL_RPMLIBS  "")
    set(OPENSSL_RPMPKGS  "")
    string(APPEND FEATURES "-openssl ")
endif()

# 3. --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(ENABLE_WEBSOCK      "${ENABLE_WEBSOCK}"      CACHE INTERNAL "Enables WebSocket features downstream")
set(GMSSL_DEBFILES      "${GMSSL_DEBFILES}"      CACHE INTERNAL "")
set(GMSSL_RPMFILES      "${GMSSL_RPMFILES}"      CACHE INTERNAL "")
set(GMSSL_CFGOPT        "${GMSSL_CFGOPT}"        CACHE INTERNAL "")
set(OPENSSL_DEBFILES    "${OPENSSL_DEBFILES}"    CACHE INTERNAL "")
set(OPENSSL_RPMFILES    "${OPENSSL_RPMFILES}"    CACHE INTERNAL "")
set(OPENSSL_CFGOPT      "${OPENSSL_CFGOPT}"      CACHE INTERNAL "")
set(OPENSSL_DEBLIBS     "${OPENSSL_DEBLIBS}"     CACHE INTERNAL "")
set(OPENSSL_DEBPKGS     "${OPENSSL_DEBPKGS}"     CACHE INTERNAL "")
set(OPENSSL_RPMLIBS     "${OPENSSL_RPMLIBS}"     CACHE INTERNAL "")
set(OPENSSL_RPMPKGS     "${OPENSSL_RPMPKGS}"     CACHE INTERNAL "")
set(FEATURES            "${FEATURES}"            CACHE INTERNAL "" FORCE)

# ==============================================================================
# --- Part 21: Package File Lists (WebSockets / FUSE3) ---
# ==============================================================================

# 1. --- WebSocket Flag Processing Loop ---
# Uses the internal ENABLE_WEBSOCK variable calculated dynamically in Part 20
if(ENABLE_WEBSOCK)
    set(WEBSOCK_RPMFILES "%{_libdir}/libwspt.so*")
else()
    set(WEBSOCK_RPMFILES "")
endif()

# 2. --- FUSE3 Flag Processing Loop ---
# Note: Replicates Autotools conditional boundary checks against ENABLE_FUSE3
if(ENABLE_FUSE3)
    set(FUSE3_RPMLIBS    "fuse3-devel,")
    set(FUSE3_RPMFILES   "%{_libdir}/libfuseif.so*ZZZZZ%{_bindir}/hostsvrZZZZZ%{_bindir}/hostcliZZZZZ%{_libdir}/rpcf/librtfiles.so*")
    set(FUSE3_RPMHEADERS "%{_includedir}/rpcf/ipc/fuseif.hZZZZZ%{_includedir}/rpcf/ipc/fusedefs.hZZZZZ%{_includedir}/rpcf/ipc/serijson.h")
    set(FUSE3_RPMPKGS    "fuse3 ")
    set(FUSE3_DEBFILES   "usr/bin/hostsvrZZZZZusr/bin/hostcliZZZZZusr/lib/*/rpcf/librtfiles.so*")
    set(FUSE3_DEBLIBS    " libfuse3-dev,")
    set(FUSE3_DEBHEADERS "")
    set(FUSE3_DEBPKGS    "fuse3,")
    set(FUSE3_CFGOPT     "")
    string(APPEND FEATURES "+fuse3 ")
else()
    set(FUSE3_RPMLIBS    "")
    set(FUSE3_RPMFILES   "")
    set(FUSE3_RPMHEADERS "")
    set(FUSE3_RPMPKGS    "")
    set(FUSE3_DEBFILES   "")
    set(FUSE3_DEBLIBS    "")
    set(FUSE3_DEBPKGS    "")
    set(FUSE3_CFGOPT     "--disable-fuse3")
    string(APPEND FEATURES "-fuse3 ")
endif()

find_package(Threads REQUIRED)

# 3. --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(WEBSOCK_RPMFILES   "${WEBSOCK_RPMFILES}"   CACHE INTERNAL "")
set(FUSE3_RPMLIBS      "${FUSE3_RPMLIBS}"      CACHE INTERNAL "")
set(FUSE3_RPMFILES     "${FUSE3_RPMFILES}"     CACHE INTERNAL "")
set(FUSE3_RPMHEADERS   "${FUSE3_RPMHEADERS}"   CACHE INTERNAL "")
set(FUSE3_RPMPKGS      "${FUSE3_RPMPKGS}"      CACHE INTERNAL "")
set(FUSE3_DEBFILES     "${FUSE3_DEBFILES}"     CACHE INTERNAL "")
set(FUSE3_DEBLIBS      "${FUSE3_DEBLIBS}"      CACHE INTERNAL "")
set(FUSE3_DEBHEADERS   "${FUSE3_DEBHEADERS}"   CACHE INTERNAL "")
set(FUSE3_DEBPKGS      "${FUSE3_DEBPKGS}"      CACHE INTERNAL "")
set(FUSE3_CFGOPT       "${FUSE3_CFGOPT}"       CACHE INTERNAL "")
set(FEATURES           "${FEATURES}"           CACHE INTERNAL "" FORCE)


# ==============================================================================
# --- Part 22: Package File Lists (Kerberos / Java Profiles) ---
# ==============================================================================

# 1. --- Kerberos (krb5) Flag Processing Loop ---
if(ENABLE_KRB5)
    set(KRB5_RPMLIBS  "krb5-devel,")
    set(KRB5_RPMPKGS  "krb5-libs")
    set(KRB5_RPMFILES "")
    set(KRB5_DEBLIBS  "libkrb5-dev,")
    set(KRB5_DEBPKGS  "libkrb5-3,")
    set(KRB5_CFGOPT   "")
    string(APPEND FEATURES "+krb5 ")
else()
    set(KRB5_RPMLIBS  "")
    set(KRB5_RPMFILES "")
    set(KRB5_RPMPKGS  "")
    set(KRB5_DEBLIBS  "")
    set(KRB5_DEBPKGS  "")
    set(KRB5_CFGOPT   "--disable-krb5")
    string(APPEND FEATURES "-krb5 ")
endif()

# 2. --- Java Flag Processing Loop ---
if(ENABLE_JAVA)
    set(JAVA_RPMLIBS   "(java >= 1.8.0 or java-latest-openjdk-devel), swig, apache-commons-cli, ")
    set(JAVA_RPMFILES  "%{_libdir}/librpcbaseJNI.so*ZZZZZ%{_libdir}/rpcf/*.jar")
    set(JAVA_RPMPKGS   "apache-commons-cli ")
    
    # Restored to normal single-bracket standard shell scripting syntax for clarity
    set(JAVA_RPMPOST   "pushd %{_libdir}/rpcf;if [[ -f rpcbase-${VERSION2}.jar ]];then rm -f rpcbase.jar; ln -s rpcbase-${VERSION2}.jar rpcbase.jar;fi;if [[ -f appmancli-${VERSION2}.jar ]];then rm -f appmancli.jar; ln -s appmancli-${VERSION2}.jar appmancli.jar;fi;popd;")
    set(JAVA_RPMPOSTUN "pushd %{_libdir}/rpcf;rm -f rpcbase.jar appmancli.jar;popd;")
    
    set(JAVA_DEBLIBS   "default-jdk-headless | openjdk-11-jdk-headless | openjdk-8-jdk-headless, swig, libcommons-cli-java, ")
    set(JAVA_DEBPKGS   "libcommons-cli-java, ")
    set(JAVA_DEBFILES  "usr/lib/*/rpcf/*.jar")
    set(JAVA_CFGOPT    "")
    string(APPEND FEATURES "+java ")
else()
    set(JAVA_RPMLIBS   "")
    set(JAVA_RPMFILES  "")
    set(JAVA_RPMPKGS   "")
    set(JAVA_RPMPOST   "")
    set(JAVA_RPMPOSTUN "")
    set(JAVA_DEBLIBS   "")
    set(JAVA_DEBFILES  "")
    set(JAVA_DEBPKGS   "")
    set(JAVA_CFGOPT    "--disable-java")
    string(APPEND FEATURES "-java ")
endif()

# 3. --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(KRB5_RPMLIBS    "${KRB5_RPMLIBS}"    CACHE INTERNAL "")
set(KRB5_RPMPKGS    "${KRB5_RPMPKGS}"    CACHE INTERNAL "")
set(KRB5_RPMFILES   "${KRB5_RPMFILES}"   CACHE INTERNAL "")
set(KRB5_DEBLIBS    "${KRB5_DEBLIBS}"    CACHE INTERNAL "")
set(KRB5_DEBPKGS    "${KRB5_DEBPKGS}"    CACHE INTERNAL "")
set(KRB5_CFGOPT     "${KRB5_CFGOPT}"     CACHE INTERNAL "")
set(JAVA_RPMLIBS    "${JAVA_RPMLIBS}"    CACHE INTERNAL "")
set(JAVA_RPMFILES   "${JAVA_RPMFILES}"   CACHE INTERNAL "")
set(JAVA_RPMPKGS    "${JAVA_RPMPKGS}"    CACHE INTERNAL "")
set(JAVA_RPMPOST    "${JAVA_RPMPOST}"    CACHE INTERNAL "")
set(JAVA_RPMPOSTUN  "${JAVA_RPMPOSTUN}"  CACHE INTERNAL "")
set(JAVA_DEBLIBS    "${JAVA_DEBLIBS}"    CACHE INTERNAL "")
set(JAVA_DEBFILES   "${JAVA_DEBFILES}"   CACHE INTERNAL "")
set(JAVA_DEBPKGS    "${JAVA_DEBPKGS}"    CACHE INTERNAL "")
set(JAVA_CFGOPT     "${JAVA_CFGOPT}"     CACHE INTERNAL "")
set(FEATURES        "${FEATURES}"        CACHE INTERNAL "" FORCE)

# ==============================================================================
# --- Part 23: Package File Lists (Python Binding Profiles) ---
# ==============================================================================

if(ENABLE_PYTHON)
    set(PYTHON_RPMLIBS   "python3-devel, python3-pip, python3-build, python3-numpy, ( sip5 or sip6 ),")
    set(PYTHON_RPMPKGS   "python3-pip python3-numpy ")
    set(PYTHON_RPMFILES  "%{_libdir}/rpcf/*.whl")
    
    # Note the escaped '\$i' to pass the literal variable out to the RPM runtime shell script
    set(PYTHON_RPMPOST   "for i in %{_libdir}/rpcf/*.whl; do pip3 install \$i; done")
    set(PYTHON_RPMPOSTUN "pip3 uninstall -y")
    
    set(PYTHON_DEBLIBS   "python3-setuptools, python3-dev, python3-build, python3-pip, sip5-tools | sip-tools | sip-dev,")
    set(PYTHON_DEBPKGS   "python3-numpy, python3-pip, ")
    set(PYTHON_DEBFILES  "usr/lib/*/rpcf/*.whl")
    set(PYTHON_CFGOPT    "")
    string(APPEND FEATURES "+python ")
else()
    set(PYTHON_RPMLIBS   "")
    set(PYTHON_RPMPKGS   "")
    set(PYTHON_RPMFILES  "")
    set(PYTHON_RPMPOST   "")
    set(PYTHON_DEBLIBS   "")
    set(PYTHON_DEBPKGS   "")
    set(PYTHON_RPMPOSTUN "hiddenvar=")
    set(PYTHON_DEBFILES  "")
    set(PYTHON_CFGOPT    "--disable-python")
    string(APPEND FEATURES "-python ")
endif()

# --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(PYTHON_RPMLIBS   "${PYTHON_RPMLIBS}"   CACHE INTERNAL "")
set(PYTHON_RPMPKGS   "${PYTHON_RPMPKGS}"   CACHE INTERNAL "")
set(PYTHON_RPMFILES  "${PYTHON_RPMFILES}"  CACHE INTERNAL "")
set(PYTHON_RPMPOST   "${PYTHON_RPMPOST}"   CACHE INTERNAL "")
set(PYTHON_DEBLIBS   "${PYTHON_DEBLIBS}"   CACHE INTERNAL "")
set(PYTHON_DEBPKGS   "${PYTHON_DEBPKGS}"   CACHE INTERNAL "")
set(PYTHON_RPMPOSTUN "${PYTHON_RPMPOSTUN}" CACHE INTERNAL "")
set(PYTHON_DEBFILES  "${PYTHON_DEBFILES}"  CACHE INTERNAL "")
set(PYTHON_CFGOPT    "${PYTHON_CFGOPT}"    CACHE INTERNAL "")


# ==============================================================================
# --- Part 24: Testcase Profiles and Node Package Listings ---
# ==============================================================================

if(ENABLE_TESTCASES)
    set(TEST_DEBFILES "etc/rpcf/actcdesc.jsonZZZZZetc/rpcf/asyndesc.jsonZZZZZetc/rpcf/evtdesc.jsonZZZZZetc/rpcf/inproc.jsonZZZZZetc/rpcf/kadesc.jsonZZZZZetc/rpcf/sfdesc.jsonZZZZZetc/rpcf/stmdesc.jsonZZZZZetc/rpcf/prdesc.json")

    # Keeps @DEB_HOST_MULTIARCH@ literal for downstream Debian packaging tools
    set(TEST_DEBBINS  "usr/bin/rpcf/actclitstZZZZZusr/bin/rpcf/actcsvrtstZZZZZusr/bin/rpcf/asynclitstZZZZZusr/bin/rpcf/asyncsvrtstZZZZZusr/bin/rpcf/evtclitstZZZZZusr/bin/rpcf/evtsvrtstZZZZZusr/bin/rpcf/inproctstZZZZZusr/bin/rpcf/kaclitstZZZZZusr/bin/rpcf/kasvrtstZZZZZusr/bin/rpcf/prclitstZZZZZusr/bin/rpcf/prsvrtstZZZZZusr/bin/rpcf/sfclitstZZZZZusr/bin/rpcf/sfsvrtstZZZZZusr/bin/rpcf/smoketestZZZZZusr/bin/rpcf/stmclitstZZZZZusr/bin/rpcf/stmsvrtstZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libactc.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libevtst.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libstmsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libinproc.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libkasvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libsfsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libprsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libascsvr.so*")

    set(TEST_RPMFILES "%{_sysconfdir}/rpcf/actcdesc.jsonZZZZZ%{_sysconfdir}/rpcf/asyndesc.jsonZZZZZ%{_sysconfdir}/rpcf/evtdesc.jsonZZZZZ%{_sysconfdir}/rpcf/inproc.jsonZZZZZ%{_sysconfdir}/rpcf/kadesc.jsonZZZZZ%{_sysconfdir}/rpcf/sfdesc.jsonZZZZZ%{_sysconfdir}/rpcf/stmdesc.jsonZZZZZ%{_sysconfdir}/rpcf/prdesc.json")

    set(TEST_RPMBINS  "%{_bindir}/rpcf/actclitstZZZZZ%{_bindir}/rpcf/actcsvrtstZZZZZ%{_bindir}/rpcf/asynclitstZZZZZ%{_bindir}/rpcf/asyncsvrtstZZZZZ%{_bindir}/rpcf/evtclitstZZZZZ%{_bindir}/rpcf/evtsvrtstZZZZZ%{_bindir}/rpcf/inproctstZZZZZ%{_bindir}/rpcf/kaclitstZZZZZ%{_bindir}/rpcf/kasvrtstZZZZZ%{_bindir}/rpcf/prclitstZZZZZ%{_bindir}/rpcf/prsvrtstZZZZZ%{_bindir}/rpcf/sfclitstZZZZZ%{_bindir}/rpcf/sfsvrtstZZZZZ%{_bindir}/rpcf/smoketestZZZZZ%{_bindir}/rpcf/stmclitstZZZZZ%{_bindir}/rpcf/stmsvrtst")

    set(TEST_CFGOPT   "")
    string(APPEND FEATURES "+testcases ")
else()
    set(TEST_DEBFILES "")
    set(TEST_DEBBINS  "")
    set(TEST_RPMFILES "")
    set(TEST_RPMBINS  "")
    set(TEST_CFGOPT   "--disable-testcases")
    string(APPEND FEATURES "-testcases ")
endif()

# --- Global Cache Variable Registration (Replaces AC_SUBST mappings) ---
set(TEST_DEBFILES      "${TEST_DEBFILES}"      CACHE INTERNAL "")
set(TEST_DEBBINS       "${TEST_DEBBINS}"       CACHE INTERNAL "")
set(TEST_RPMFILES      "${TEST_RPMFILES}"      CACHE INTERNAL "")
set(TEST_RPMBINS       "${TEST_RPMBINS}"       CACHE INTERNAL "")
set(TEST_CFGOPT        "${TEST_CFGOPT}"        CACHE INTERNAL "")
set(JS_NODE_PACKAGES   "${JS_NODE_PACKAGES}"   CACHE INTERNAL "JavaScript node runtime modules list")


# ==============================================================================
# --- Part 25: Package File Lists (JavaScript Profiles & Features Close) ---
# ==============================================================================

# Replaces JS_NODE_PACKAGES initialization
set(JS_NODE_PACKAGES "assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack webpack-cli minify vm events crypto-browserify stream-browserify")

if(ENABLE_JS)
    set(JS_RPMLIBS      "npm,")
    set(JS_RPMFILES     "%{_libdir}/rpcf/jslib/*ZZZZZ%{_sysconfdir}/rpcf/oa2checkdesc.jsonZZZZZ%{_mandir}/man1/oinit.1.gzZZZZZ%{_sysconfdir}/rpcf/appmonui.tar.gz")
    
    # Natively substitutes the JS_NODE_PACKAGES string variable we defined in Part 24
    set(JS_RPMPOST      "echo npm -g install ${JS_NODE_PACKAGES}")
    set(JS_RPMPOSTUN    "echo npm -g remove ${JS_NODE_PACKAGES}")
    
    set(JS_DEBLIBS      "npm,")
    set(JS_DEBDEVFILES  "usr/lib/@DEB_HOST_MULTIARCH@/rpcf/jslib/*")
    set(JS_DEBFILES     "etc/rpcf/oa2checkdesc.jsonZZZZZusr/share/man/man1/oinit.1ZZZZZetc/rpcf/appmonui.tar.gz")
    set(JS_CFGOPT       "")
    string(APPEND FEATURES "+js ")
else()
    set(JS_RPMLIBS      "")
    set(JS_RPMFILES     "")
    set(JS_RPMPOST      "")
    set(JS_DEBLIBS      "")
    set(JS_DEBDEVFILES  "")
    set(JS_RPMPOSTUN    "")
    set(JS_DEBFILES     "")
    set(JS_CFGOPT       "--disable-js")
    string(APPEND FEATURES "-js ")
endif()


# ------------------------------------------------------------------------------
# --- Global Optimization: Saving & Exporting FEATURES ---
# ------------------------------------------------------------------------------

# 1. Replaces CXXFLAGS+=' -DFEATURES="..."'
# Passing the final accumulated string dynamically to your compiler definitions.
add_compile_definitions(FEATURES="${FEATURES}")

# Ensure the raw text string is cleanly appended for legacy downstream .pc or Makefile templates
set(CXXFLAGS "${CXXFLAGS} -DFEATURES=\\\"\"${FEATURES}\"\\\"")

# 2. Replaces AC_SUBST mappings (Global Cache Registration)
set(JS_RPMLIBS      "${JS_RPMLIBS}"      CACHE INTERNAL "")
set(JS_RPMFILES     "${JS_RPMFILES}"     CACHE INTERNAL "")
set(JS_RPMPOST      "${JS_RPMPOST}"      CACHE INTERNAL "")
set(JS_RPMPOSTUN    "${JS_RPMPOSTUN}"    CACHE INTERNAL "")
set(JS_DEBLIBS      "${JS_DEBLIBS}"      CACHE INTERNAL "")
set(JS_DEBDEVFILES  "${JS_DEBDEVFILES}"  CACHE INTERNAL "")
set(JS_DEBFILES     "${JS_DEBFILES}"     CACHE INTERNAL "")
set(JS_CFGOPT       "${JS_CFGOPT}"       CACHE INTERNAL "")

# SAVE THE ACCUMULATED FEATURES TO THE CACHE ONCE HERE (As planned in Part 22!)
set(FEATURES        "${FEATURES}"        CACHE INTERNAL "Finalized features tracking string" FORCE)
set(CXXFLAGS        "${CXXFLAGS}"        CACHE INTERNAL "" FORCE)


# ==============================================================================
# --- Part 26: Operating System Detection & Compilation Flags (The End) ---
# ==============================================================================

set(BUILD_LINUX   OFF)
set(BUILD_WINDOWS OFF)
set(BUILD_MAC     OFF)

# Replaces the $host_os case statement using CMAKE_SYSTEM_NAME
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(BUILD_LINUX ON)
    add_compile_definitions(LINUX)
    set(CXXFLAGS "${CXXFLAGS} -DLINUX")

# Matches cygwin* or mingw* environment spaces
elseif(CMAKE_SYSTEM_NAME MATCHES "^(Windows|MSYS|CYGWIN|MinGW)")
    set(BUILD_WINDOWS ON)
    add_compile_definitions(WINDOWS)
    set(CXXFLAGS "${CXXFLAGS} -DWINDOWS")

# Matches darwin* environment spaces (macOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(BUILD_MAC ON)
    add_compile_definitions(OSX)
    set(CXXFLAGS "${CXXFLAGS} -DOSX")

else()
    # Replaces AC_MSG_ERROR(["OS $host_os is not supported"])
    message(FATAL_ERROR "****** OS ${CMAKE_SYSTEM_NAME} is not supported by this project ******")
endif()

# --- Global Cache Variable Registration (Replaces AM_CONDITIONAL mappings) ---
set(BUILD_LINUX   "${BUILD_LINUX}"   CACHE INTERNAL "Targeting Linux platform")
set(BUILD_WINDOWS "${BUILD_WINDOWS}" CACHE INTERNAL "Targeting Windows platform")
set(BUILD_MAC     "${BUILD_MAC}"     CACHE INTERNAL "Targeting macOS platform")
set(CXXFLAGS      "${CXXFLAGS}"      CACHE INTERNAL "" FORCE)

# ==============================================================================
# --- Part 18: File Configuration & Generation (Replaces AC_CONFIG_FILES) ---
# ==============================================================================
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_BUGREPORT "woodhead99@gmail.com" )
set(VERSION "${PROJECT_VERSION}")

if(NOT CMAKE_LIBRARY_ARCHITECTURE)
    execute_process(
        COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
        OUTPUT_VARIABLE DEB_HOST_MULTIARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(DEB_HOST_MULTIARCH "${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

# 1. --- Core pkg-config file generation ---
# Replaces: rpcf.pc inside AC_CONFIG_FILES
# This reads 'rpcf.pc.in' and generates a tailored 'rpcf.pc' inside your build directory.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/rpcf.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/rpcf.pc"
    @ONLY
)

# 2. --- Script, Dockerfile, and Configuration File Substitutions ---
# Replaces all non-Makefile items in your AC_CONFIG_FILES list.
# For these to compile, make sure to append a '.in' extension to their real files
# (e.g., rename 'updcfg.py' to 'updcfg.py.in' if it contains @VARIABLE@ hooks).

set(CONFIG_FILES
    "tools/Dockerfile"
    "tools/Dockerfile.arm"
    "tools/updcfg.py"
    "tools/buildall-fed.sh"
    "tools/buildall-deb.sh"
    "tools/buildall-rasp.sh"
    "bldpkg/control"
    "bldpkg/rules"
    "bldpkg/rpcf.spec"
    "bldpkg/rpcf.install"
    "bldpkg/rpcf-dev.install"
    "bldpkg/rpcf-static.install"
    "bldpkg/blddpkg.sh"
    "python/setup.py"
    "monitor/oinit/setup.py"
    "monitor/oinit/oinit.spec"
    "monitor/client/python/build_deb.sh"
    "monitor/client/python/build_rpm.sh"
    "monitor/client/python/appmancli/setup.py"
    "monitor/client/python/appmancli/pyproject.toml"
    "monitor/client/js/appmoncli/Makefile.skel"
)

foreach(FILE IN LISTS CONFIG_FILES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.in")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${FILE}"
            @ONLY
        )
    endif()
endforeach()

# ==============================================================================
# --- FINALIZATION GENERATION STEP ---
# ==============================================================================

# --- Verification Message ---
message(STATUS "----------------------------------------------------")
message(STATUS "  RPC-Frmwrk Configuration Completed Successfully")
message(STATUS "  Target Bit-width : ${BITWIDTH}-bit")
message(STATUS "  Target Processor : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "----------------------------------------------------")


# --- Subdirectories (SUBDIRS translation) ---
add_subdirectory(3rd_party)
add_subdirectory(combase)
add_subdirectory(ipc)
add_subdirectory(docs)

if(ENABLE_FUSE3)
    add_subdirectory(fuse)
endif()

# Combines the nested SIPBUILD/SIP4 and ENABLE_PYTHON conditionals
if((SIPBUILD AND ENABLE_PYTHON) OR (SIP4 AND ENABLE_PYTHON))
    add_subdirectory(python)
endif()

add_subdirectory(ridl)
add_subdirectory(test)

if(SWIG AND ENABLE_JAVA)
    add_subdirectory(java)
endif()

if(ENABLE_JS)
    add_subdirectory(js)
endif()

add_subdirectory(monitor)
add_subdirectory(logger)
add_subdirectory(rpc)
add_subdirectory(tools)
add_subdirectory(pics)
add_subdirectory(examples)
add_subdirectory(bldpkg)

# --- Dist/Source files documentation ---
add_custom_target(extra_dist_files SOURCES 
    README.md 
    cfgsel 
    Concept.md 
    logs.md 
    LICENSE 
    README_cn.md 
    Concept_cn.md
)

# ==============================================================================
# --- Bottom Half Translation (Pkg-Config & Packaging Loops) ---
# ==============================================================================

# 1. dist_pkgconfig_DATA=rpcf.pc
# Installs your generated or existing .pc file to the standard pkgconfig location
include(GNUInstallDirs)
install(FILES rpcf.pc 
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)

# Replicates the Autotools $(subst -,_,...) string replacement for Debian naming conventions
string(REPLACE "-" "_" DEBTAR "${DISTDIR}")
set(DEBTAR "${DEBTAR}.orig.tar.gz")


find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache compiler optimization accelerator discovered natively!")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_SKIP_INSTALL_RPATH ON)

# the custom target 'dist'
add_custom_target(dist
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/bldpkg/collect_files.sh" "${CMAKE_BINARY_DIR}/${PKGNAME}" "${DISTDIR}"
        && echo "Successfully generated distribution package ${CMAKE_BINARY_DIR}/${PKGNAME}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)

# =====================================================================
# building deb/rpm package
# =====================================================================
set(IS_IDEAL_DEB_HOST FALSE)
set(IS_IDEAL_RPM_HOST FALSE)

# 1. Check if we are running on a Linux platform
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    # 2. Inspect the system's OS identification file
    if(EXISTS "/etc/os-release")
        file(READ "/etc/os-release" OS_RELEASE_CONTENT)
        
        # Check if the OS ID matches debian, ubuntu, or raspbian
        if(OS_RELEASE_CONTENT MATCHES "ID=(debian|ubuntu|raspbian)")
            set(IS_IDEAL_DEB_HOST TRUE)
        elseif(OS_RELEASE_CONTENT MATCHES "ID=(fedora|centos|redhat)")
            set(IS_IDEAL_RPM_HOST TRUE)
        endif()
    endif()
endif()

# 3. Query the standard tool dependencies
find_program(AUTOCONF_EXE NAMES autoconf)
find_program(AUTOMAKE_EXE NAMES automake)
find_program(DPKG_BUILD_EXE NAMES dpkg-buildpackage)
find_program(RPM_BUILD_EXE NAMES rpm-build)

# 4. Final approval: Must have both the tools AND the right OS host
if(IS_IDEAL_DEB_HOST AND AUTOCONF_EXE AND AUTOMAKE_EXE AND DPKG_BUILD_EXE)
    set(AUTOTOOLS_DEB_AVAILABLE TRUE)
else()
    set(AUTOTOOLS_DEB_AVAILABLE FALSE)
endif()

if(IS_IDEAL_RPM_HOST AND AUTOCONF_EXE AND AUTOMAKE_EXE AND RPM_BUILD_EXE)
    set(AUTOTOOLS_RPM_AVAILABLE TRUE)
else()
    set(AUTOTOOLS_RPM_AVAILABLE FALSE)
endif()

# =====================================================================
# DEFINE TARGETS
# =====================================================================
if(AUTOTOOLS_DEB_AVAILABLE)

# 3. Custom 'deb' Target
# Replicates the original shell script behavior exactly using CMake-native commands
add_custom_target(deb
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target dist
    COMMAND echo "dist package is built"
    COMMAND ${CMAKE_COMMAND} -E rename "${PKGNAME}" "bldpkg/${DEBTAR}"
    COMMAND ${CMAKE_COMMAND} -E chdir bldpkg ${CMAKE_COMMAND} -E tar zxf "${DEBTAR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "bldpkg/${DISTDIR}-${REV}"
    COMMAND ${CMAKE_COMMAND} -E rename "bldpkg/${DISTDIR}" "bldpkg/${DISTDIR}-${REV}"
    COMMAND ${CMAKE_COMMAND} --build bldpkg --target deb_rpcf -- DESTDIR=./bldpkg/${DISTDIR}-${REV}/debian/${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "bldpkg/${DISTDIR}-${REV}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building Debian package..."
)
endif()
if(AUTOTOOLS_RPM_AVAILABLE)
# 4. Custom 'rpm' Target
add_custom_target(rpm
    COMMAND ${CMAKE_COMMAND} --build bldpkg --target rpm_rpcf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building RPM package..."
)
endif()

include(CPack)
