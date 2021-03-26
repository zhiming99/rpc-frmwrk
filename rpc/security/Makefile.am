lib_LTLIBRARIES=libauth.la

rpcfauth_headers = k5proxy.h k5server.h kdcfdo.h secclsid.h secfido.h security.h README.md
libauth_la_SOURCES = security.cpp dllexp.cpp kdcfdo.cpp secfido.cpp k5proxy.cpp k5server.cpp $(rpcfauth_headers)

libauth_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ \
-I$(top_srcdir)/combase -I$(top_srcdir)/rpc -I$(top_srcdir)/3rd_party

libauth_ladir=$(libdir)

libauth_conf_DATA=authprxy.json
libauth_confdir=$(sysconfdir)/rpcf

libauth_la_LDFLAGS = -version-info 0:0:0
libauth_la_LIBADD = $(COMMON_LINK) -L$(top_srcdir)/combase -L.. -L$(top_srcdir)/ipc -L$(top_srcdir)/3rd_party -lutils -lrpc -lipc -lcombase -lpthread -ldl

