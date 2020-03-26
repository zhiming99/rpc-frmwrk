#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpc-frmwrk], [0.1], [woodhead99@gmail.com])
AC_CONFIG_SRCDIR([ipc/rpcif.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_SYSTEM

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([.])

AM_INIT_AUTOMAKE([foreign])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([combase], [DllLoadFactory])
AC_CHECK_LIB([ipc], [DllLoadFactory])
AC_CHECK_LIB([rpc], [DllLoadFactory])
AC_CHECK_LIB([dbus-1], [dbus_bus_get])
AC_CHECK_LIB([jsoncpp], [_ZNK4Json5ValuecvbEv])
AC_CHECK_LIB([lz4], [LZ4_compress_default])
AC_CHECK_LIB([ssl], [SSL_do_handshake])

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

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

#use --with-sysroot to set the cross-compile's rootfs
AC_SUBST(with_sysroot)
AC_SUBST(INCLUDES)

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime memmove ftruncate getcwd gettimeofday inet_ntoa memset realpath socket strerror strtol])

AC_CONFIG_FILES([Makefile
                 Makefile.start
                 Makefile.init.pub
                 combase/Makefile
                 ipc/Makefile
                 rpc/Makefile
                 rpc/sslport/Makefile
                 rpc/wsport/Makefile
                 rpc/wsport/wsproto/Makefile
                 test/Makefile
                 test/Makefile.init.pub
                 test/actcancel/Makefile
                 test/asynctst/Makefile
                 test/evtest/Makefile
                 test/helloworld/Makefile
                 test/inproc/Makefile
                 test/prtest/Makefile
                 test/katest/Makefile
                 test/sftest/Makefile
                 test/stmtest/Makefile
                 test/router/Makefile
                 test/iftest/Makefile
                 test/btinrt/Makefile
                 test/smoketest/Makefile])
AC_OUTPUT
