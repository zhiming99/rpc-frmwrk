#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpcfrmwrk],[0.1],[woodhead99@gmail.com])
AC_CONFIG_SRCDIR([ipc/rpcif.cpp]) AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([.])

AM_INIT_AUTOMAKE([foreign])

#: ${CXXFLAGS=""}
#: ${CFLAGS=""}
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_SED
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_INSTALL

AC_LANG([C++])
#AX_CXX_COMPILE_STDCXX(11,[noext], [mandatory])

AC_CHECK_PROG(ac_cv_prog_python3, python3, yes)
LT_INIT

# Checks for libraries.
AC_CHECK_LIB([combase], [DllLoadFactory])
AC_CHECK_LIB([dbus-1], [dbus_bus_get])
AC_CHECK_LIB([jsoncpp], [_ZNK4Json5ValuecvbEv])
AC_CHECK_LIB([lz4], [LZ4_compress_default])
AC_CHECK_LIB([ssl], [SSL_do_handshake])
AC_CHECK_LIB([gssapi_krb5], [gss_wrap])
AC_CHECK_LIB([krb5], [krb5_init_context])
AC_CHECK_LIB([atomic], [__atomic_fetch_add_8])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h \
                fcntl.h \
                netinet/in.h \
                stdlib.h \
                string.h \
                sys/socket.h \
                sys/time.h \
                unistd.h \
                lz4.h \
                openssl/ssl.h])

AC_CHECK_HEADERS([dbus/dbus.h])
AC_CHECK_HEADERS([glib.h])
AC_CHECK_HEADERS([json/json.h])
AC_CHECK_HEADERS([cppunit/ui/text/TestRunner.h])
AC_CHECK_HEADERS([libintl.h limits.h \
netdb.h malloc.h stddef.h sys/ioctl.h  ])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_CHECK_TYPES([ptrdiff_t])
#use --with-sysroot to set the cross-compile's rootfs
AC_SUBST(with_sysroot)
#AC_SUBST(INCLUDES)

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_REALLOC
AC_FUNC_ERROR_AT_LINE

AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_PROG_RANLIB
AC_CHECK_FUNCS([clock_gettime memmove ftruncate \
getcwd gettimeofday inet_ntoa memset realpath \
socket strerror strtol strcasecmp strchr strtoul \
strtoull ])

AC_TYPE_INT16_T 
AC_TYPE_INT32_T 
AC_TYPE_INT64_T 
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T 

AC_DEFINE(_USE_LIBEV, 1, [using built-in event loop])
AC_DEFINE(_GNU_SOURCE, 1,[compatible with old build])

AC_SUBST( CXXFLAGS, '`pkg-config --cflags dbus-1 \
jsoncpp cppunit` \
-I$(top_srcdir)/include -I$(top_srcdir)/3rd_party' )

AC_SUBST( TSTLDFLG, '-lipc -lcombase -lstdc++ -lcppunit \
-L$(top_srcdir)/3rd_party -L$(top_srcdir)/combase -L$(top_srcdir)/ipc' )

AC_CONFIG_FILES([Makefile
                 combase/Makefile
                 ipc/Makefile
                 rpc/Makefile
                 rpc/sslport/Makefile
                 rpc/wsport/Makefile
                 rpc/wsport/wsproto/Makefile
                 rpc/security/Makefile
                 rpc/router/Makefile
                 ridl/Makefile
                 3rd_party/Makefile
                 python/Makefile
                 test/Makefile
                 test/actcancel/Makefile
                 test/asynctst/Makefile
                 test/evtest/Makefile
                 test/helloworld/Makefile
                 test/inproc/Makefile
                 test/prtest/Makefile
                 test/katest/Makefile
                 test/sftest/Makefile
                 test/stmtest/Makefile
                 test/iftest/Makefile
                 test/btinrt/Makefile
                 test/smoketest/Makefile])
AC_OUTPUT
