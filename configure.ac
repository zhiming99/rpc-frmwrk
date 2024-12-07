#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpcf],[0.7.0],[woodhead99@gmail.com],[rpcf],[https://github.com/zhiming99/rpc-frmwrk])
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

AC_ARG_ENABLE([krb5],
    [AS_HELP_STRING([--disable-krb5], [build and install without Kerberos authentication])],
    [enable_krb5=no],
    [enable_krb5=yes])

AC_ARG_ENABLE([gmssl],
    [AS_HELP_STRING([--disable-gmssl], [build and install without GmSSL])],
    [enable_gmssl=no],
    [enable_gmssl=yes])

AC_ARG_ENABLE([fuse3],
    [AS_HELP_STRING([--disable-fuse3], [build and install without fuse3])],
    [enable_fuse3=no],
    [enable_fuse3=yes])

AC_ARG_ENABLE([java],
    [AS_HELP_STRING([--disable-java], [no generation of Java skeleton code])],
    [enable_java=no],
    [enable_java=yes])

AC_ARG_ENABLE([python],
    [AS_HELP_STRING([--disable-python], [no generation of python skeleton code])],
    [enable_python=no],
    [enable_python=yes])


AC_ARG_ENABLE([js],
    [AS_HELP_STRING([--disable-js], [no generation of JavaScript skeleton code])],
    [enable_js=no],
    [enable_js=yes])

AC_CHECK_PROG([swig], [swig], yes)

case $enable_gmssl in
    yes)
    AS_IF(
        [test -f "/usr/local/include/gmssl/sm2.h" -a -f "/usr/local/lib/libgmssl.a" ], 
        [AC_MSG_RESULT([checking for gmssl... yes])],
        [enable_gmssl=no]
    )
    ;;
    no)
    ;;
esac

case $enable_python in
    yes)
    AC_CHECK_PROG([python3], python3, yes)
    AC_CHECK_PROG([sipbuild], [sip-build], yes)
    AC_CHECK_PROG([sip4], [sip], yes)
    ;;
    no)
    ;;
esac

case $enable_js in
    yes)
    AC_CHECK_PROG([npm], npm, yes)
    ;;
    no)
    ;;
esac

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

case $enable_krb5 in
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

case $enable_krb5 in
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
                 tools/updcfg.py
                 3rd_party/Makefile
                 pics/Makefile
                 bldpkg/Makefile
                 bldpkg/control
                 bldpkg/rules
                 bldpkg/rpcf.spec
                 bldpkg/rpcf.install
                 bldpkg/rpcf-dev.install
                 examples/Makefile
                 test/Makefile
                 test/iftest/Makefile
                 test/btinrt/Makefile
                 test/actcancel/Makefile
                 test/asynctst/Makefile
                 test/evtest/Makefile
                 test/helloworld/Makefile
                 test/inproc/Makefile
                 test/prtest/Makefile
                 test/katest/Makefile
                 test/sftest/Makefile
                 test/stmtest/Makefile
                 test/smoketest/Makefile
                 rpc/gmsslport/Makefile
                 rpc/sslport/Makefile
                 rpc/wsport/Makefile 
                 rpc/wsport/wsproto/Makefile
                 rpc/security/Makefile
                 rpc/security/oa2check/Makefile
                 java/Makefile
                 fuse/Makefile
                 python/Makefile
                 python/setup.py
                 js/Makefile
                 logger/Makefile
                 monitor/registry/Makefile
                 monitor/oinit/Makefile
                 monitor/oinit/setup.py
                 monitor/Makefile
                 docs/Makefile
                 rpcf.pc ])

AM_CONDITIONAL([ENABLE_TESTCASES],[test x$enable_testcases = xyes])
AM_CONDITIONAL([ENABLE_OPENSSL],[test x$enable_openssl = xyes])
AM_CONDITIONAL([ENABLE_KRB5],[test x$enable_krb5 = xyes])
AM_CONDITIONAL([ENABLE_AUTH],[test x$enable_krb5 = xyes -o x$enable_js = xyes ])
AM_CONDITIONAL([ENABLE_GMSSL],[test x$enable_gmssl = xyes])
AM_CONDITIONAL([ENABLE_FUSE3],[test x$ac_cv_lib_fuse3_fuse_mount = xyes -a x$enable_fuse3 = xyes])
AM_CONDITIONAL([SIPBUILD],[test x$ac_cv_prog_sipbuild = xyes])
AM_CONDITIONAL([SIP4],[test x$ac_cv_prog_sip4 = xyes])
AM_CONDITIONAL([SWIG],[test x$ac_cv_prog_swig = xyes])
AM_CONDITIONAL([ENABLE_JAVA],[test x$ac_cv_prog_javac = xyes -a x$enable_java = xyes])
AM_CONDITIONAL([ENABLE_PYTHON],[test \( "x$ac_cv_prog_sip4" = "xyes" -o "x$ac_cv_prog_sipbuild" = "xyes" \) -a "x$enable_python" = "xyes"])
AM_CONDITIONAL([BITWIDTH32],[test x$BITWIDTH = x32])
AM_CONDITIONAL([ENABLE_JS],[test x$ac_cv_prog_npm = xyes -a x$enable_js=xyes ])

VERSION2='$(subst .,:,$(VERSION))'
AC_SUBST(VERSION2)

FEATURES=

enable_websock=no
case $enable_gmssl in
    yes)
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
        OPENSSL_DEBFILES='usr/bin/rpcf/opensslkey.shZZZZZetc/rpcf/openssl.cnf'
        OPENSSL_RPMFILES='%{_bindir}/rpcf/opensslkey.shZZZZZ%{_libdir}/libsslpt.so*ZZZZZ%{_sysconfdir}/rpcf/openssl.cnf'
        OPENSSL_CFGOPT=
        OPENSSL_DEBLIBS='libssl-dev,'
        OPENSSL_DEBPKGS='openssl, libssl1.1 | libssl3, expect,'
        OPENSSL_RPMLIBS='openssl-devel,'
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
        WEBSOCK_RPMFILES='%{_libdir}/libwspt.so*'
    ;;
    no)
        WEBSOCK_RPMFILES=
    ;;
esac

case $enable_fuse3 in
    yes)
        FUSE3_RPMLIBS="fuse3-devel,"
        FUSE3_RPMFILES='%{_libdir}/libfuseif.so*ZZZZZ%{_bindir}/hostsvrZZZZZ%{_bindir}/hostcliZZZZZ%{_libdir}/rpcf/librtfiles.so*'
        FUSE3_RPMHEADERS='%{_includedir}/rpcf/ipc/fuseif.hZZZZZ%{_includedir}/rpcf/ipc/fusedefs.hZZZZZ%{_includedir}/rpcf/ipc/serijson.h'
        FUSE3_RPMPKGS="fuse3 fuse3-libs"
        FUSE3_DEBFILES="usr/bin/hostsvrZZZZZusr/bin/hostcliZZZZZusr/lib/*/rpcf/librtfiles.so*"
        FUSE3_DEBLIBS=" libfuse3-dev,"
        FUSE3_DEBHEADERS=
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

case $enable_krb5 in
    yes)
        KRB5_RPMLIBS="krb5-devel,"
        KRB5_RPMPKGS="krb5-libs" 
        KRB5_RPMFILES=
        KRB5_DEBLIBS="libkrb5-dev,"
        KRB5_DEBPKGS="libkrb5-3,"
        KRB5_CFGOPT=
        FEATURES+="+krb5 "
    ;;
    no)
        KRB5_RPMLIBS=
        KRB5_RPMFILES=
        KRB5_RPMPKGS=
        KRB5_DEBLIBS=
        KRB5_DEBPKGS=
        KRB5_CFGOPT='--disable-krb5'
        FEATURES+="-krb5 "
    ;;
esac

case $enable_java in
    yes)
        JAVA_RPMLIBS="java-1.8.0-openjdk-devel, java-1.8.0-openjdk-headless, java-1.8.0-openjdk, swig, apache-commons-cli, " 
        JAVA_RPMFILES="%{_libdir}/librpcbaseJNI.so*ZZZZZ%{_libdir}/rpcf/rpcbase.jar"
        JAVA_RPMPKGS="java-1.8.0-openjdk-headless apache-commons-cli " 
        JAVA_DEBLIBS="openjdk-8-jdk-headless | openjdk-8-jdk | openjdk-11-jdk-headless | openjdk-11-jdk | default-jdk, swig, libcommons-cli-java, "
        JAVA_DEBPKGS="openjdk-8-jdk-headless | openjdk-8-jdk | openjdk-11-jdk-headless | openjdk-11-jdk | default-jdk, libcommons-cli-java, "
        JAVA_DEBFILES="usr/lib/*/rpcf/*.jar"
        JAVA_CFGOPT=
        FEATURES+="+java "
    ;;
    no)
        JAVA_RPMLIBS=
        JAVA_RPMFILES=
        JAVA_RPMPKGS=
        JAVA_DEBLIBS=
        JAVA_DEBFILES=
        JAVA_DEBPKGS=
        JAVA_CFGOPT='--disable-java'
        FEATURES+="-java "
    ;;
esac

case $enable_python in
    yes)
        PYTHON_RPMLIBS="python3-devel, python3-pip, python3-numpy, ( sip5 or sip6 )," 
        PYTHON_RPMPKGS="python3-pip python3-numpy (sip5 or sip6) " 
        PYTHON_RPMFILES="%{_libdir}/rpcf/*.whl"
        PYTHON_RPMPOST="for i in %{_libdir}/rpcf/*.whl; do pip3 install \$i; done"
        PYTHON_RPMPOSTUN="pip3 uninstall -y"
        PYTHON_DEBLIBS="python3-setuptools, python3-dev, python3-pip, sip5-tools | sip-dev | sip-tools,"
        PYTHON_DEBPKGS="python3-numpy, python3-pip, sip5-tools | sip-tools, " 
        PYTHON_DEBFILES="usr/lib/*/rpcf/*.whl"
        PYTHON_CFGOPT=
        FEATURES+="+python "
    ;;
    no)
        PYTHON_RPMLIBS=
        PYTHON_RPMPKGS=
        PYTHON_RPMFILES=
        PYTHON_RPMPOST=
        PYTHON_DEBLIBS=
        PYTHON_DEBPKGS=
        PYTHON_RPMPOSTUN="hiddenvar="
        PYTHON_DEBFILES=
        PYTHON_CFGOPT='--disable-python'
        FEATURES+="-python "
    ;;
esac

case $enable_testcases in
    yes)
        TEST_DEBFILES='etc/rpcf/actcdesc.jsonZZZZZetc/rpcf/asyndesc.jsonZZZZZetc/rpcf/evtdesc.jsonZZZZZetc/rpcf/inproc.jsonZZZZZetc/rpcf/kadesc.jsonZZZZZetc/rpcf/sfdesc.jsonZZZZZetc/rpcf/stmdesc.jsonZZZZZetc/rpcf/prdesc.json'
        TEST_DEBBINS='usr/bin/rpcf/actclitstZZZZZusr/bin/rpcf/actcsvrtstZZZZZusr/bin/rpcf/asynclitstZZZZZusr/bin/rpcf/asyncsvrtstZZZZZusr/bin/rpcf/evtclitstZZZZZusr/bin/rpcf/evtsvrtstZZZZZusr/bin/rpcf/inproctstZZZZZusr/bin/rpcf/kaclitstZZZZZusr/bin/rpcf/kasvrtstZZZZZusr/bin/rpcf/prclitstZZZZZusr/bin/rpcf/prsvrtstZZZZZusr/bin/rpcf/sfclitstZZZZZusr/bin/rpcf/sfsvrtstZZZZZusr/bin/rpcf/smoketestZZZZZusr/bin/rpcf/stmclitstZZZZZusr/bin/rpcf/stmsvrtstZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libactc.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libevtst.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libstmsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libinproc.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libkasvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libsfsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libprsvr.so*ZZZZZusr/lib/@DEB_HOST_MULTIARCH@/rpcf/libascsvr.so*'

        TEST_RPMFILES='%{_sysconfdir}/rpcf/actcdesc.jsonZZZZZ%{_sysconfdir}/rpcf/asyndesc.jsonZZZZZ%{_sysconfdir}/rpcf/evtdesc.jsonZZZZZ%{_sysconfdir}/rpcf/inproc.jsonZZZZZ%{_sysconfdir}/rpcf/kadesc.jsonZZZZZ%{_sysconfdir}/rpcf/sfdesc.jsonZZZZZ%{_sysconfdir}/rpcf/stmdesc.jsonZZZZZ%{_sysconfdir}/rpcf/prdesc.json'

        TEST_RPMBINS='%{_bindir}/rpcf/actclitstZZZZZ%{_bindir}/rpcf/actcsvrtstZZZZZ%{_bindir}/rpcf/asynclitstZZZZZ%{_bindir}/rpcf/asyncsvrtstZZZZZ%{_bindir}/rpcf/evtclitstZZZZZ%{_bindir}/rpcf/evtsvrtstZZZZZ%{_bindir}/rpcf/inproctstZZZZZ%{_bindir}/rpcf/kaclitstZZZZZ%{_bindir}/rpcf/kasvrtstZZZZZ%{_bindir}/rpcf/prclitstZZZZZ%{_bindir}/rpcf/prsvrtstZZZZZ%{_bindir}/rpcf/sfclitstZZZZZ%{_bindir}/rpcf/sfsvrtstZZZZZ%{_bindir}/rpcf/smoketestZZZZZ%{_bindir}/rpcf/stmclitstZZZZZ%{_bindir}/rpcf/stmsvrtst'

        TEST_CFGOPT=
        FEATURES+="+testcases "
    ;;
    no)
        TEST_DEBFILES=
        TEST_DEBBINS=
        TEST_RPMFILES=
        TEST_RPMBINS=
        TEST_CFGOPT='--disable-testcases'
        FEATURES+="-testcases "
    ;;
esac

case $enable_js in
    yes)
        JS_RPMLIBS="npm," 
        JS_RPMFILES="%{_sysconfdir}/rpcf/jslib/*ZZZZZ%{_sysconfdir}/rpcf/oa2checkdesc.json"
        #JS_RPMPOST="mkdir -p %{_sysconfdir}/rpcf/jslib && cd %{_sysconfdir}/rpcf/jslib && tar xf ../rpcbase.js.tar.gz;"
        JS_RPMPOST="npm -g install assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack webpack-cli minify vm events crypto-browserify stream-browserify"
        #JS_RPMPOSTUN="cd %{_sysconfdir}/rpcf && rm -rf ./jslib"
        JS_RPMPOSTUN="npm -g remove assert browserify buffer exports long lz4 process put safe-buffer stream xxhashjs xxhash webpack webpack-cli minify vm events crypto-browserify stream-browserify"
        JS_DEBLIBS="npm,"
        JS_DEBDEVFILES="etc/rpcf/oa2check.ridlZZZZZetc/rpcf/jslib/*"
        JS_DEBFILES="etc/rpcf/oa2checkdesc.json"
        JS_CFGOPT=
        FEATURES+="+js "
    ;;
    no)
        JS_RPMLIBS=
        JS_RPMFILES=
        JS_RPMPOST=
        JS_DEBLIBS=
        JS_DEBDEVFILES=
        JS_RPMPOSTUN=
        JS_DEBFILES=
        JS_CFGOPT='--disable-js'
        FEATURES+="-js "
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
AC_SUBST(FUSE3_DEBHEADERS)


AC_SUBST(KRB5_RPMLIBS)
AC_SUBST(KRB5_RPMFILES)
AC_SUBST(KRB5_RPMPKGS)
AC_SUBST(KRB5_DEBLIBS)
AC_SUBST(KRB5_DEBPKGS)
AC_SUBST(KRB5_CFGOPT)

AC_SUBST(JAVA_RPMLIBS)
AC_SUBST(JAVA_RPMPKGS)
AC_SUBST(JAVA_RPMFILES)
AC_SUBST(JAVA_DEBLIBS)
AC_SUBST(JAVA_DEBFILES)
AC_SUBST(JAVA_DEBPKGS)
AC_SUBST(JAVA_CFGOPT)

AC_SUBST(PYTHON_RPMLIBS)
AC_SUBST(PYTHON_RPMPKGS)
AC_SUBST(PYTHON_RPMFILES)
AC_SUBST(PYTHON_RPMPOST)
AC_SUBST(PYTHON_DEBLIBS)
AC_SUBST(PYTHON_RPMPOSTUN)
AC_SUBST(PYTHON_DEBFILES)
AC_SUBST(PYTHON_CFGOPT)
AC_SUBST(PYTHON_DEBPKGS)

AC_SUBST(TEST_DEBFILES)
AC_SUBST(TEST_DEBBINS)
AC_SUBST(TEST_RPMFILES)
AC_SUBST(TEST_RPMBINS)
AC_SUBST(TEST_CFGOPT)

AC_SUBST(JS_RPMLIBS)
AC_SUBST(JS_RPMFILES)
AC_SUBST(JS_RPMPOST)
AC_SUBST(JS_DEBLIBS)
AC_SUBST(JS_DEBDEVFILES)
AC_SUBST(JS_RPMPOSTUN)
AC_SUBST(JS_DEBFILES)
AC_SUBST(JS_CFGOPT)

AC_CANONICAL_HOST

build_linux=no
build_windows=no
build_mac=no

# Detect the target system
case $host_os in
    linux*)
        build_linux=yes
        CXXFLAGS+=" -DLINUX"
        ;;
    cygwin*|mingw*)
        build_windows=yes
        CXXFLAGS+=" -DWINDOWS"
        ;;
    darwin*)
        build_mac=yes
        CXXFLAGS+=" -DOSX"
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac

AM_CONDITIONAL([LINUX], [test x$build_linux = yes])
AM_CONDITIONAL([WINDOWS], [test x$build_windows = yes])
AM_CONDITIONAL([OSX], [test x$build_mac = yes])

AC_OUTPUT
