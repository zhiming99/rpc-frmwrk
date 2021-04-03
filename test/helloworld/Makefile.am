include_ipcdir=$(top_srcdir)/ipc
iftest_LDFLAGS=@TSTLDFLG@
iftest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libhellowrd.la
libhellowrd_la_SOURCES = ifsvr.cpp ifclient.cpp ifsvr.h
libhellowrd_la_LDFLAGS=-version-info @VERSION2@
libhellowrd_la_LIBADD= $(iftest_LDFLAGS) -lpthread -ldl
libhellowrd_la_CPPFLAGS=$(iftest_CPPFLAGS)

iftest_PROGRAMS=hwsvrsmk hwclismk
iftestdir=$(bindir)

hwclismk_DATA= hwdesc.json
hwclismkdir=$(sysconfdir)/rpcf

hwclismk_SOURCES= iftest.cpp iftest.h
hwclismk_LDADD=$(iftest_LDFLAGS) libhellowrd.la
hwclismk_CPPFLAGS=$(iftest_CPPFLAGS) -DCLIENT

hwsvrsmk_SOURCES=iftest.cpp iftest.h
hwsvrsmk_LDADD=$(iftest_LDFLAGS) libhellowrd.la
hwsvrsmk_CPPFLAGS=$(iftest_CPPFLAGS) -DSERVER

link_dir=./
