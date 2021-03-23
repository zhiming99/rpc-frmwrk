include_ipcdir=$(top_srcdir)/ipc
async_LDFLAGS=@TSTLDFLG@
async_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libascsvr.la
libascsvr_la_SOURCES = asyncsvr.cpp asyncli.cpp
libascsvr_la_LDFLAGS=-version-info 0:0:0
libascsvr_la_LIBADD= -lpthread -ldl
libascsvr_la_CPPFLAGS=$(async_CPPFLAGS)

asynctst_PROGRAMS=asyncsvrtst asynclitst
asynctstdir=$(bindir)

asynclitst_DATA= asyndesc.json
asynclitstdir=$(sysconfdir)/rpcf

asynclitst_SOURCES= asynctest.cpp
asynclitst_LDADD=$(async_LDFLAGS) libascsvr.la
asynclitst_CPPFLAGS=$(async_CPPFLAGS) -DCLIENT

asyncsvrtst_SOURCES=asynctest.cpp
asyncsvrtst_LDADD=$(async_LDFLAGS) libascsvr.la
asyncsvrtst_CPPFLAGS=$(async_CPPFLAGS) -DSERVER

link_dir=./


