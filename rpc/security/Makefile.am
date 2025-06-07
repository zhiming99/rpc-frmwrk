lib_LTLIBRARIES=libauth.la

libauth_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/ipc -I../ \
 -I$(top_srcdir)/combase -I$(top_srcdir)/rpc -I$(top_srcdir)/3rd_party \
 -I$(top_srcdir)/monitor/appmon -I$(top_srcdir)/monitor/client/cpp/appmancli \
 -I$(top_srcdir)/monitor/sinit

if ENABLE_KRB5
libauth_la_CPPFLAGS += -DKRB5
krb5_sources=kdcfdo.cpp secfido.cpp k5proxy.cpp k5server.cpp 
else
krb5_sources=
endif

SUBDIRS =
if ENABLE_JS
libauth_la_CPPFLAGS += -DOA2 -I../../monitor/registry
SUBDIRS += oa2check
OA2Lib = -L./oa2check -loa2check 
OA2Login = ../../monitor/registry/libregfs.la
OA2_sources = oa2proxy.cpp
OA2_headers = oa2proxy.h
endif
SUBDIRS += .

rpcfauth_headers = k5proxy.h k5server.h kdcfdo.h secclsid.h secfido.h security.h $(OA2_headers) saproxy.h sacheck.h
libauth_la_SOURCES = security.cpp dllexp.cpp $(rpcfauth_headers) $(krb5_sources) $(OA2_sources) saproxy.cpp sacheck.cpp

libauth_ladir=$(libdir)

dist_libauth_conf_DATA=blist.json
libauth_confdir=$(sysconfdir)/rpcf

EXTRA_DIST=README.md README_cn.md

libauth_la_LDFLAGS = -version-info @VERSION2@
libauth_la_LIBADD = -L$(top_srcdir)/combase -L.. -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party -lutils $(OA2Lib) $(OA2Login)\
../librpc.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK_KRB5) \
 $(top_srcdir)/monitor/client/cpp/appmancli/libappmancli.la \
 $(top_srcdir)/monitor/sinit/librpcfenc.la

