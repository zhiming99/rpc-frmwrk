include_ipcdir=$(top_srcdir)/ipc
async_LDFLAGS=@TSTLDFLG@
async_CPPFLAGS= -I$(include_ipcdir) -fPIC

libascsvr_LTLIBRARIES=libascsvr.la
libascsvrdir=$(libdir)/rpcf
libascsvr_la_SOURCES = asyncsvr.cpp asyncli.cpp asyncsvr.h
libascsvr_la_LDFLAGS=-version-info @VERSION2@
libascsvr_la_LIBADD= $(async_LDFLAGS) -lpthread -ldl
libascsvr_la_CPPFLAGS=$(async_CPPFLAGS)

asynctst_PROGRAMS=asyncsvrtst asynclitst
asynctstdir=$(bindir)/rpcf

dist_asynclitst_DATA= asyndesc.json
asynclitstdir=$(sysconfdir)/rpcf

asynclitst_SOURCES= asynctest.cpp asynctest.h
asynclitst_LDADD=$(async_LDFLAGS) libascsvr.la
asynclitst_CPPFLAGS=$(async_CPPFLAGS) -DCLIENT

asyncsvrtst_SOURCES=asynctest.cpp asynctest.h
asyncsvrtst_LDADD=$(async_LDFLAGS) libascsvr.la
asyncsvrtst_CPPFLAGS=$(async_CPPFLAGS) -DSERVER

link_dir=./


