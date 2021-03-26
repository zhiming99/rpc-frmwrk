include_ipcdir=$(top_srcdir)/ipc
actc_LDFLAGS=@TSTLDFLG@
actc_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libactc.la
libactc_la_SOURCES = actcsvr.cpp actcli.cpp actcsvr.h
libactc_la_LDFLAGS=-version-info 0:0:0
libactc_la_LIBADD= -lpthread -ldl
libactc_la_CPPFLAGS=$(actc_CPPFLAGS)

actctst_PROGRAMS=actcsvrtst actclitst
actctstdir=$(bindir)

actclitst_DATA= actcdesc.json
actclitstdir=$(sysconfdir)/rpcf

actclitst_SOURCES= actctest.cpp actctest.h
actclitst_LDADD=$(actc_LDFLAGS) libactc.la
actclitst_CPPFLAGS=$(actc_CPPFLAGS) -DCLIENT

actcsvrtst_SOURCES=actctest.cpp actctest.h
actcsvrtst_LDADD=$(actc_LDFLAGS) libactc.la
actcsvrtst_CPPFLAGS=$(actc_CPPFLAGS) -DSERVER

link_dir=./


