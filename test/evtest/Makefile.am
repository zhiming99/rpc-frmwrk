include_ipcdir=$(top_srcdir)/ipc
evtest_LDFLAGS=@TSTLDFLG@
evtest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libevtst.la
libevtst_la_SOURCES = evtsvr.cpp evtcli.cpp evtsvr.h
libevtst_la_LDFLAGS=-version-info @VERSION@
libevtst_la_LIBADD= $(evtest_LDFLAGS) -lpthread -ldl
libevtst_la_CPPFLAGS=$(evtest_CPPFLAGS)

evtest_PROGRAMS=evtsvrtst evtclitst
evtestdir=$(bindir)

evtclitst_DATA=evtdesc.json
evtclitstdir=$(sysconfdir)/rpcf

evtclitst_SOURCES= evtest.cpp evtest.h
evtclitst_LDADD=$(evtest_LDFLAGS) libevtst.la
evtclitst_CPPFLAGS=$(evtest_CPPFLAGS) -DCLIENT

evtsvrtst_SOURCES=evtest.cpp evtest.h
evtsvrtst_LDADD=$(evtest_LDFLAGS) libevtst.la
evtsvrtst_CPPFLAGS=$(evtest_CPPFLAGS) -DSERVER

link_dir=./
