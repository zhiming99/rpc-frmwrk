include_ipcdir=$(top_srcdir)/ipc
iftest_LDFLAGS=@TSTLDFLG@
iftest_CPPFLAGS= -I$(include_ipcdir) -fPIC

libecho_LTLIBRARIES=libecho.la
libechodir=$(libdir)/rpcf
libecho_la_SOURCES = ifsvr.cpp ifclient.cpp ifsvr.h
libecho_la_LDFLAGS=-version-info @VERSION2@
libecho_la_LIBADD=$(iftest_LDFLAGS) -lpthread -ldl
libecho_la_CPPFLAGS=$(iftest_CPPFLAGS)

iftest_PROGRAMS=ifsvrsmk ifclismk
iftestdir=$(bindir)/rpcf

dist_ifclismk_DATA= echodesc.json
ifclismkdir=$(sysconfdir)/rpcf

ifclismk_SOURCES= iftest.cpp iftest.h
ifclismk_LDADD=$(iftest_LDFLAGS) libecho.la
ifclismk_CPPFLAGS=$(iftest_CPPFLAGS) -DCLIENT

ifsvrsmk_SOURCES=iftest.cpp iftest.h
ifsvrsmk_LDADD=$(iftest_LDFLAGS) libecho.la
ifsvrsmk_CPPFLAGS=$(iftest_CPPFLAGS) -DSERVER

link_dir=./
