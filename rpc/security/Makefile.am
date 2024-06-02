lib_LTLIBRARIES=libauth.la

libauth_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/ipc -I../ \
-I$(top_srcdir)/combase -I$(top_srcdir)/rpc -I$(top_srcdir)/3rd_party

if ENABLE_KRB5
libauth_la_CPPFLAGS += -DKRB5
krb5_sources=kdcfdo.cpp secfido.cpp k5proxy.cpp k5server.cpp 
else
krb5_sources=
endif

SUBDIRS =
if ENABLE_JS
libauth_la_CPPFLAGS += -DOA2
SUBDIRS += oa2check
OA2Lib = -L./oa2check -loa2check
endif
SUBDIRS += .

rpcfauth_headers = k5proxy.h k5server.h kdcfdo.h secclsid.h secfido.h security.h README.md 
libauth_la_SOURCES = security.cpp dllexp.cpp $(rpcfauth_headers) $(krb5_sources)

libauth_ladir=$(libdir)

dist_libauth_conf_DATA=
libauth_confdir=$(sysconfdir)/rpcf

libauth_la_LDFLAGS = -version-info @VERSION2@
libauth_la_LIBADD = -L$(top_srcdir)/combase -L.. -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party -lutils $(OA2Lib) \
../librpc.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK_KRB5) 

