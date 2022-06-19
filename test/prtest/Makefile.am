include_ipcdir=$(top_srcdir)/ipc
prtest_LDFLAGS=@TSTLDFLG@
prtest_CPPFLAGS= -I$(include_ipcdir) -fPIC

libprsvr_LTLIBRARIES= libprsvr.la
libprsvrdir=$(libdir)/rpcf
libprsvr_la_SOURCES = prsvr.cpp prcli.cpp prsvr.h
libprsvr_la_LDFLAGS=-version-info @VERSION2@
libprsvr_la_LIBADD= $(prtest_LDFLAGS) -lpthread -ldl
libprsvr_la_CPPFLAGS=$(prtest_CPPFLAGS)

prtest_PROGRAMS= prsvrtst prclitst
prtestdir=$(bindir)/rpcf

dist_prclitst_DATA= prdesc.json
prclitstdir=$(sysconfdir)/rpcf

prclitst_SOURCES= prtest.cpp prtest.h
prclitst_LDADD=$(prtest_LDFLAGS) libprsvr.la
prclitst_CPPFLAGS=$(prtest_CPPFLAGS) -DCLIENT

prsvrtst_SOURCES= prtest.cpp prtest.h
prsvrtst_LDADD=$(prtest_LDFLAGS) libprsvr.la
prsvrtst_CPPFLAGS=$(prtest_CPPFLAGS) -DSERVER

link_dir=./
