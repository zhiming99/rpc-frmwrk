include(CheckIncludeFile)
include(CheckTypeSize)

# 1. --- Replaces AC_CHECK_LIB([atomic], [__atomic_fetch_add_8], ...) ---
# Checks if your platform requires an explicit link to libatomic for 64-bit operations
find_library(ATOMIC_LIB NAMES atomic)
if(ATOMIC_LIB)
    set(HAVE_LIBatomic 1)
endif()

# 2. --- Replaces AC_CHECK_HEADERS([arpa/inet.h ...]) ---
# Iterates through your core list and maps definitions like HAVE_ARPA_INET_H
set(CORE_HEADERS
    "arpa/inet.h" "fcntl.h" "netinet/in.h" "stdlib.h" 
    "string.h" "sys/socket.h" "sys/time.h" "unistd.h" "lz4.h"
    "dbus/dbus.h" "glib.h" "json/json.h" "cppunit/ui/text/TestRunner.h"
    "libintl.h" "limits.h" "netdb.h" "malloc.h" "stddef.h" "sys/ioctl.h"
)

foreach(HEADER IN LISTS CORE_HEADERS)
    # Convert path dividers like '/' or '.' to clean macro names (e.g., HAVE_SYS_SOCKET_H)
    string(MAKE_C_IDENTIFIER "HAVE_${HEADER}" VAR_NAME)
    string(TOUPPER "${VAR_NAME}" VAR_NAME)
    check_include_file("${HEADER}" ${VAR_NAME})
endforeach()

# 3. --- Replaces AC_CHECK_HEADER_STDBOOL & AC_C_INLINE ---
# Obsolete in modern C++11 engines, but mapped to maintain absolute source compatibility
set(HAVE_STDBOOL_H 1)
set(inline inline)

# 4. --- Replaces AC_TYPE_OFF_T, SIZE_T, SSIZE_T, ptrdiff_t ---
check_type_size("off_t" SIZEOF_OFF_T)
if(SIZEOF_OFF_T)
    set(HAVE_OFF_T 1)
endif()

check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("ssize_t" SIZEOF_SSIZE_T)
if(SIZEOF_SSIZE_T)
    set(HAVE_SSIZE_T 1)
endif()

check_type_size("ptrdiff_t" SIZEOF_PTRDIFF_T)
if(SIZEOF_PTRDIFF_T)
    set(HAVE_PTRDIFF_T 1)
endif()

# 5. --- Replaces AC_SUBST(with_sysroot) ---
# Cross-compiling rootfs injection is supported natively by CMake out of the box!
# If passed by the user, we expose it globally
if(with_sysroot)
    set(CMAKE_SYSROOT "${with_sysroot}" CACHE PATH "Cross-compilation sysroot target" FORCE)
endif()

include(CheckFunctionExists)

# 1. --- Replaces AC_FUNC_MALLOC & AC_FUNC_REALLOC ---
# Autotools checks if malloc(0) returns a valid pointer. If not, it replaces it
# with rpl_malloc. In modern C++11, we assume standard-compliant behavior.
set(HAVE_MALLOC 1)
set(HAVE_REALLOC 1)

# 2. --- Replaces AC_FUNC_ALLOCA, ERROR_AT_LINE, STRTOD ---
check_include_file("alloca.h" HAVE_ALLOCA_H)
check_function_exists(error_at_line HAVE_ERROR_AT_LINE)
check_function_exists(strtod HAVE_STRTOD)

# 3. --- Replaces AC_CHECK_FUNCS([clock_gettime ...]) ---
set(CORE_FUNCTIONS
    "clock_gettime" "memmove" "ftruncate" "getcwd" "gettimeofday"
    "inet_ntoa" "memset" "realpath" "socket" "strerror" "strtol"
    "strcasecmp" "strchr" "strtoul" "strtoull"
)

foreach(FUNC IN LISTS CORE_FUNCTIONS)
    string(TOUPPER "HAVE_${FUNC}" VAR_NAME)
    check_function_exists("${FUNC}" ${VAR_NAME})
endforeach()

# 4. --- Replaces AC_TYPE_INT16_T, UINT32_T, etc. ---
# These are guaranteed by C++11 (<cstdint>), but we define them for absolute
# source compatibility with legacy #ifdef guards.
set(HAVE_INT16_T 1)
set(HAVE_INT32_T 1)
set(HAVE_INT64_T 1)
set(HAVE_INT8_T 1)
set(HAVE_MODE_T 1)
set(HAVE_UINT16_T 1)
set(HAVE_UINT32_T 1)
set(HAVE_UINT64_T 1)
set(HAVE_UINT8_T 1)

# 5. --- Replaces AC_PROG_RANLIB ---
# Completely obsolete. CMake handles static library indexing (ranlib)
# natively out-of-the-box on all platforms.



