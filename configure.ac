#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpcf],[0.5.0],[woodhead99@gmail.com])
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

AC_ARG_ENABLE([ssl],
    [AS_HELP_STRING([--disable-ssl], [build and install without SSL support])],
    [enable_gmssl=no, enable_openssl=no],
    [])

AC_ARG_ENABLE([testcases],
    [AS_HELP_STRING([--disable-testcases], [build and install without testcases])],
    [enable_testcases=no],
    [enable_testcases=yes])

AC_ARG_ENABLE([openssl],
    [AS_HELP_STRING([--disable-openssl], [build and install without openssl])],
    [enable_openssl=no],
    [enable_openssl=yes])

AC_ARG_ENABLE([auth],
    [AS_HELP_STRING([--disable-auth], [build and install without authentication])],
    [enable_auth=no],
    [enable_auth=yes])

AC_ARG_ENABLE([gmssl],
    [AS_HELP_STRING([--disable-gmssl], [build and install without GmSSL])],
    [enable_gmssl=no],
    [enable_gmssl=yes])

AC_ARG_ENABLE([fuse3],
    [AS_HELP_STRING([--disable-fuse3], [build and install without fuse3])],
    [enable_fuse3=no],
    [enable_fuse3=yes])

AC_ARG_ENABLE([java],
    [AS_HELP_STRING([--disable-java], [no generation of Java skelton code])],
    [enable_java=no],
    [enable_java=yes])

AC_CHECK_PROG([python3], python3, yes)
AC_CHECK_PROG([sipbuild], [sip-build], yes)
AC_CHECK_PROG([sip4], [sip], yes)
AC_CHECK_PROG([swig], [swig], yes)

case $enable_java in
    yes)
    AC_CHECK_PROG([javac],[javac],yes)
    ;;
    no)
    ;;
esac

LT_INIT

# Checks for libraries.
#AC_CHECK_LIB([combase], [DllLoadFactory])
AC_CHECK_LIB([dbus-1], [dbus_bus_get],
[LIBS="${LIBS}"
AC_DEFINE([HAVE_LIBdbus_1], [1], [Define to 1 if you have dbus-1.]) ],
[AC_MSG_ERROR([dbus-1 was not found])] )

AC_CHECK_LIB([lz4], [LZ4_compress_default],
[LIBS="${LIBS}"
AC_DEFINE([HAVE_LIBlz4], [1], [Define to 1 if you have lz4.])],
[AC_MSG_ERROR([lz4 was not found])] )

AC_CHECK_LIB([jsoncpp], [_ZNK4Json5ValuecvbEv],
[LIBS="${LIBS}"
AC_DEFINE([HAVE_LIBjsoncpp], [1], [Define to 1 if you have jsoncpp.])])

case $enable_openssl in
    yes)

    AC_CHECK_LIB([ssl], [SSL_do_handshake],
    [LIBS="${LIBS}"
    AC_DEFINE([HAVE_LIBssl], [1], [Define to 1 if you have ssl.])])
    AC_CHECK_HEADERS([openssl/ssl.h])
    ;;
    no)
    ;;
esac

case $enable_auth in
    yes)
    AC_CHECK_LIB([gssapi_krb5], [gss_wrap],
    [LIBS="${LIBS}"
    AC_DEFINE([HAVE_LIBgssapi], [1], [Define to 1 if you have krb5-gssapi.])])

    AC_CHECK_LIB([krb5], [krb5_init_context],
    [LIBS="${LIBS}"
    AC_DEFINE([HAVE_LIBkrb5], [1], [Define to 1 if you have krb5.])])
    AC_CHECK_HEADERS([gssapi/gssapi_krb5.h])
    ;;
    no)
    ;;
esac

case $enable_fuse3 in
    yes)
    AC_CHECK_LIB([fuse3], [fuse_mount],
    [LIBS="${LIBS}"
    AC_DEFINE([HAVE_LIBfuse3], [1], [Define to 1 if you have fuse3.]) ],
    [AC_MSG_ERROR([fuse3 was not found])] )
    ;;
    no)
    ;;
esac

AC_CHECK_LIB([atomic], [__atomic_fetch_add_8],
[LIBS="${LIBS}"
AC_DEFINE([HAVE_LIBatomic], [1], [Define to 1 if you have atomic.])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h \
                fcntl.h \
                netinet/in.h \
                stdlib.h \
                string.h \
                sys/socket.h \
                sys/time.h \
                unistd.h \
                lz4.h 
                ])

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

DEPLIBS_ALL='dbus-1 jsoncpp cppunit'

case $enable_openssl in
    yes)
    DEPLIBS_ALL+=' openssl'
    ;;
    no)
    ;;
esac

case $enable_fuse3 in
    yes)
    case $ac_cv_lib_fuse3_fuse_mount in
        yes)
            DEPLIBS_ALL+=' fuse3'
        ;;
        no)
        ;;
        esac
    ;;
    no)
    ;;
esac

case $enable_auth in
    yes)
    DEPLIBS_ALL+=' mit-krb5'
    ;;
    no)
    ;;
esac



DEPLIBS='dbus-1 jsoncpp'
DEPLIBS_CPPUNIT='dbus-1 jsoncpp cppunit'
DEPLIBS_SSL='dbus-1 jsoncpp openssl'
DEPLIBS_KRB5='dbus-1 jsoncpp mit-krb5 mit-krb5-gssapi'
DEPLIBS_FUSE3='dbus-1 jsoncpp fuse3'

AC_SUBST(DEPLIBS)
AC_SUBST(DEPLIBS_ALL)
AC_SUBST(DEPLIBS_CPPUNIT)
AC_SUBST(DEPLIBS_SSL)
AC_SUBST(DEPLIBS_KRB5)
AC_SUBST(DEPLIBS_FUSE3)

case ${with_sysroot} in
    no)
    CXXFLAGS='`pkg-config --cflags ${DEPLIBS_ALL}` -D_USE_LIBEV -I$(top_srcdir)/include -I$(top_srcdir)/3rd_party'
    COMMON_LINK='`pkg-config --libs ${DEPLIBS}`'
    COMMON_LINK_CPPUNIT='`pkg-config --libs ${DEPLIBS_CPPUNIT}`'
    COMMON_LINK_SSL='`pkg-config --libs ${DEPLIBS_SSL}`'
    COMMON_LINK_KRB5='`pkg-config --libs ${DEPLIBS_KRB5}`'
    COMMON_LINK_FUSE3='`pkg-config --libs ${DEPLIBS_FUSE3}`'
    SYSROOT=
    ;;

    *)
    PKG_CONFIG_PATH=
    PKG_CONFIG_LIBDIR=$(find ${with_sysroot} -type d -name pkgconfig | tr "\n" ":" )
    PKG_CONFIG_SYSROOT_DIR=${with_sysroot}

    SYSROOT=${with_sysroot}
    CXXFLAGS='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} \
    pkg-config --cflags ${DEPLIBS_ALL}` -D_USE_LIBEV -I$(top_srcdir)/include -I$(top_srcdir)/3rd_party'

    COMMON_LINK='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} \
    pkg-config --libs ${DEPLIBS}`'

    COMMON_LINK_CPPUNIT='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} \
    PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} pkg-config --libs \
    ${DEPLIBS_CPPUNIT}`'
    COMMON_LINK_SSL='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} \
    PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} pkg-config --libs \
    ${DEPLIBS_SSL}`'
    COMMON_LINK_KRB5='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} \
    PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} pkg-config --libs \
    ${DEPLIBS_KRB5}`'
    COMMON_LINK_FUSE3='`PKG_CONFIG_SYSROOT_DIR=${PKG_CONFIG_SYSROOT_DIR} \
    PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR} pkg-config --libs \
    ${DEPLIBS_FUSE3}`'

    AC_SUBST(PKG_CONFIG_PATH)
    AC_SUBST(PKG_CONFIG_LIBDIR)
    AC_SUBST(PKG_CONFIG_SYSROOT_DIR)

    ;;
esac

AC_SUBST(COMMON_LINK)
AC_SUBST(COMMON_LINK_CPPUNIT)
AC_SUBST(COMMON_LINK_SSL)
AC_SUBST(COMMON_LINK_KRB5)
AC_SUBST(COMMON_LINK_FUSE3)

AC_SUBST(SYSROOT)

AC_MSG_NOTICE([the cxxflags string is ${CXXFLAGS}])

case $host in
    arm*)
        CXXFLAGS+=' -DARM'
        COMMON_LINK+=' -latomic'
        COMMON_LINK_CPPUNIT+=' -latomic'
        COMMON_LINK_SSL+=' -latomic'
        COMMON_LINK_KRB5+=' -latomic'
        COMMON_LINK_FUSE3+=' -latomic'
    ;;
    aarch64)
        CXXFLAGS+=' -DARM'
        COMMON_LINK+=' -latomic'
        COMMON_LINK_CPPUNIT+=' -latomic'
        COMMON_LINK_SSL+=' -latomic'
        COMMON_LINK_KRB5+=' -latomic'
        COMMON_LINK_FUSE3+=' -latomic'
    ;;
esac

case $host in
    x86_64*)
        BITWIDTH=64
        CXXFLAGS+=' -DBUILD_64=1 -m64'
    ;;
    aarch64*)
        BITWIDTH=64
        CXXFLAGS+=' -DBUILD_64=1'
    ;;
    arm64*)
        BITWIDTH=64
        CXXFLAGS+=' -DBUILD_64=1'
    ;;
    arm*)
        BITWIDTH=32
        CXXFLAGS+=' -DBUILD_64=0'
    ;;
    i?86*)
        BITWIDTH=32
        CXXFLAGS+=' -DBUILD_64=0 -m32'
    ;;
esac

CXXFLAGS+=' -std=c++11'
CXXFLAGS+=' -DVERSION_STRING="\"$(VERSION)\""'

AC_SUBST([BITWIDTH])

AC_DEFINE(_GNU_SOURCE, 1,[GNU non-portable features])

AC_SUBST( TSTLDFLG, '$(top_srcdir)/ipc/libipc.la $(top_srcdir)/combase/libcombase.la -lstdc++ -lcppunit \
-L$(top_srcdir)/3rd_party -L$(top_srcdir)/combase -L$(top_srcdir)/ipc \
${COMMON_LINK_CPPUNIT}' )

PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

AC_CONFIG_FILES([Makefile
                 combase/Makefile
                 ipc/Makefile
                 rpc/Makefile
                 rpc/router/Makefile
                 ridl/Makefile
                 ridl/rpcfs/Makefile
                 tools/Makefile
                 3rd_party/Makefile
                 python/Makefile
                 python/setup.py
                 pics/Makefile
                 bldpkg/Makefile
                 bldpkg/control
                 bldpkg/rules
                 bldpkg/rpcf.spec
                 bldpkg/rpcf.install
                 bldpkg/rpcf-dev.install
                 examples/Makefile
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
                 test/smoketest/Makefile
                 rpcf.pc ])

AM_CONDITIONAL([ENABLE_TESTCASES],[test x$enable_testcases = xyes])
AM_CONDITIONAL([ENABLE_OPENSSL],[test x$enable_openssl = xyes])
AM_CONDITIONAL([ENABLE_AUTH],[test x$enable_auth = xyes])
AM_CONDITIONAL([ENABLE_GMSSL],[test x$enable_gmssl = xyes])
AM_CONDITIONAL([ENABLE_FUSE3],[test x$ac_cv_lib_fuse3_fuse_mount = xyes -a x$enable_fuse3 = xyes])
AM_CONDITIONAL([SIPBUILD],[test x$ac_cv_prog_sipbuild = xyes])
AM_CONDITIONAL([SIP4],[test x$ac_cv_prog_sip4 = xyes])
AM_CONDITIONAL([SWIG],[test x$ac_cv_prog_swig = xyes])
AM_CONDITIONAL([ENABLE_JAVA],[test x$ac_cv_prog_javac = xyes -a x$enable_java = xyes])
AM_CONDITIONAL([BITWIDTH32],[test x$BITWIDTH = x32])

VERSION2='$(subst .,:,$(VERSION))'
AC_SUBST(VERSION2)

FEATURES=

enable_websock=no
case $enable_gmssl in
    yes)
        AC_CONFIG_FILES([
             rpc/gmsslport/Makefile ])
        GMSSL_DEBFILES='usr/bin/rpcf/gmsslkey.sh'
        GMSSL_RPMFILES='%{_bindir}/rpcf/gmsslkey.shZZZZZ%{_libdir}/libgmsslpt.so*'
        GMSSL_CFGOPT=
        enable_websock=yes
        FEATURES+="+gmssl "
    ;;
    no)
        GMSSL_DEBFILES=
        GMSSL_RPMFILES=
        GMSSL_CFGOPT='--disable-gmssl'
        FEATURES+="-gmssl "
    ;;
esac

case $enable_openssl in
    yes)
        AC_CONFIG_FILES([
             rpc/sslport/Makefile ])
        OPENSSL_DEBFILES='usr/bin/rpcf/opensslkey.shZZZZZetc/rpcf/openssl.cnf'
        OPENSSL_RPMFILES='%{_bindir}/rpcf/opensslkey.shZZZZZ%{_libdir}/libsslpt.so*ZZZZZ%{_sysconfdir}/rpcf/openssl.cnf'
        OPENSSL_CFGOPT=
        OPENSSL_DEBLIBS='libssl-dev,'
        OPENSSL_DEBPKGS='openssl, libssl1.1, expect,'
        OPENSSL_RPMLIBS='openssl-devel'
        OPENSSL_RPMPKGS='openssl openssl-libs expect'
        enable_websock=yes
        FEATURES+="+openssl "
    ;;
    no)
        OPENSSL_DEBFILES=
        OPENSSL_RPMFILES=
        OPENSSL_CFGOPT='--disable-openssl'
        OPENSSL_DEBLIBS=
        OPENSSL_DEBPKGS=
        OPENSSL_RPMLIBS=
        OPENSSL_RPMPKGS=
        FEATURES+="-openssl "
    ;;
esac

case $enable_websock in
    yes)
        AC_CONFIG_FILES([
             rpc/wsport/Makefile 
             rpc/wsport/wsproto/Makefile ])
        WEBSOCK_RPMFILES='%{_libdir}/libwspt.so*'
    ;;
    no)
        WEBSOCK_RPMFILES=
    ;;
esac

case $enable_fuse3 in
    yes)
        AC_CONFIG_FILES([
             fuse/Makefile ])
        FUSE3_RPMLIBS="fuse3-devel"
        FUSE3_RPMFILES='%{_libdir}/libfuseif.so*ZZZZZ%{_bindir}/hostsvrZZZZZ%{_bindir}/hostcli'
        FUSE3_RPMHEADERS='%{_includedir}/rpcf/ipc/fuseif.hZZZZZ%{_includedir}/rpcf/ipc/fusedefs.hZZZZZ%{_includedir}/rpcf/ipc/serijson.h'
        FUSE3_RPMPKGS="fuse3 fuse3-libs"
        FUSE3_DEBFILES="usr/bin/hostsvrZZZZZusr/bin/hostcli"
        FUSE3_DEBLIBS=" libfuse3-dev,"
        FUSE3_DEBPKGS="fuse3,libfuse3-3,"
        FUSE3_CFGOPT=
        FEATURES+="+fuse3 "
    ;;
    no)
        FUSE3_RPMLIBS=
        FUSE3_RPMFILES=
        FUSE3_RPMHEADERS=
        FUSE3_RPMPKGS=
        FUSE3_DEBFILES=
        FUSE3_DEBLIBS=
        FUSE3_DEBPKGS=
        FUSE3_CFGOPT='--disable-fuse3'
        FEATURES+="-fuse3 "
    ;;
esac

case $enable_auth in
    yes)
        AC_CONFIG_FILES([
             rpc/security/Makefile ])
        AUTH_RPMLIBS="krb5-devel"
        AUTH_RPMFILES="%{_libdir}/libauth.so*"
        AUTH_RPMPKGS="krb5-libs" 
        AUTH_DEBLIBS="libkrb5-dev,"
        AUTH_DEBPKGS="libkrb5-3,"
        AUTH_CFGOPT=
        FEATURES+="+auth "
    ;;
    no)
        AUTH_RPMLIBS=
        AUTH_RPMFILES=
        AUTH_RPMPKGS=
        AUTH_DEBLIBS=
        AUTH_DEBPKGS=
        AUTH_CFGOPT='--disable-auth'
        FEATURES+="-auth "
    ;;
esac

case $enable_java in
    yes)
        AC_CONFIG_FILES([
                 java/Makefile ])
        JAVA_RPMLIBS="java-1.8.0-openjdk-devel java-1.8.0-openjdk-headless java-1.8.0-openjdk" 
        JAVA_RPMPKGS=
        JAVA_RPMFILES="%{_libdir}/librpcbaseJNI.so*ZZZZZ%{_libdir}/rpcf/rpcbase.jar"
        JAVA_RPMHEADERS="%{_sysconfdir}/rpcf/JavaSerialBase.javaZZZZZ%{_sysconfdir}/rpcf/JavaSerialImpl.java"
        JAVA_DEBLIBS="openjdk-8-jdk-headless | openjdk-8-jdk | openjdk-11-jdk-headless | openjdk-11-jdk | default-jdk,"
        JAVA_DEBFILES="usr/lib/*/rpcf/*.jar"
        JAVA_DEBPKGS=
        JAVA_DEBHEADERS="etc/rpcf/*.java"
        JAVA_CFGOPT=
        FEATURES+="+java "
    ;;
    no)
        JAVA_RPMLIBS=
        JAVA_RPMPKGS=
        JAVA_RPMFILES=
        JAVA_DEBLIBS=
        JAVA_DEBPKGS=
        JAVA_DEBFILES=
        JAVA_DEBHEADERS=
        JAVA_CFGOPT='--disable-java'
        FEATURES+="-java "
    ;;
esac

CXXFLAGS+=' -DFEATURES="\"$(FEATURES)\""'

AC_SUBST([CXXFLAGS])
AC_SUBST(FEATURES)
AC_SUBST(GMSSL_DEBFILES)
AC_SUBST(GMSSL_RPMFILES)
AC_SUBST(GMSSL_CFGOPT)

AC_SUBST(OPENSSL_DEBFILES)
AC_SUBST(OPENSSL_RPMFILES)
AC_SUBST(OPENSSL_CFGOPT)
AC_SUBST(OPENSSL_DEBLIBS)
AC_SUBST(OPENSSL_DEBPKGS)
AC_SUBST(OPENSSL_RPMLIBS)
AC_SUBST(OPENSSL_RPMPKGS)
AC_SUBST(WEBSOCK_RPMFILES)

AC_SUBST(FUSE3_DEBFILES)
AC_SUBST(FUSE3_RPMFILES)
AC_SUBST(FUSE3_CFGOPT)
AC_SUBST(FUSE3_DEBLIBS)
AC_SUBST(FUSE3_DEBPKGS)
AC_SUBST(FUSE3_RPMLIBS)
AC_SUBST(FUSE3_RPMPKGS)
AC_SUBST(FUSE3_RPMHEADERS)

AC_SUBST(AUTH_RPMLIBS)
AC_SUBST(AUTH_RPMFILES)
AC_SUBST(AUTH_RPMPKGS)
AC_SUBST(AUTH_DEBLIBS)
AC_SUBST(AUTH_DEBPKGS)
AC_SUBST(AUTH_CFGOPT)

AC_SUBST(JAVA_RPMLIBS)
AC_SUBST(JAVA_RPMPKGS)
AC_SUBST(JAVA_RPMFILES)
AC_SUBST(JAVA_RPMHEADERS)
AC_SUBST(JAVA_DEBLIBS)
AC_SUBST(JAVA_DEBPKGS)
AC_SUBST(JAVA_DEBFILES)
AC_SUBST(JAVA_DEBHEADERS)
AC_SUBST(JAVA_CFGOPT)

AC_OUTPUT
