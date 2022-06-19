include_ipcdir=$(top_srcdir)/ipc
actc_LDFLAGS=@TSTLDFLG@
actc_CPPFLAGS= -I$(include_ipcdir)

libactc_LTLIBRARIES=libactc.la
libactcdir=$(libdir)/rpcf
libactc_la_SOURCES = actcsvr.cpp actcli.cpp actcsvr.h
libactc_la_LDFLAGS=-version-info @VERSION2@
libactc_la_LIBADD= $(actc_LDFLAGS) -lpthread -ldl
libactc_la_CPPFLAGS=$(actc_CPPFLAGS)

actctst_PROGRAMS=actcsvrtst actclitst
actctstdir=$(bindir)/rpcf

dist_actclitst_DATA= actcdesc.json
actclitstdir=$(sysconfdir)/rpcf

actclitst_SOURCES= actctest.cpp actctest.h
actclitst_LDADD=$(actc_LDFLAGS) libactc.la
actclitst_CPPFLAGS=$(actc_CPPFLAGS) -DCLIENT

actcsvrtst_SOURCES=actctest.cpp actctest.h
actcsvrtst_LDADD=$(actc_LDFLAGS) libactc.la
actcsvrtst_CPPFLAGS=$(actc_CPPFLAGS) -DSERVER -fPIC

link_dir=./


