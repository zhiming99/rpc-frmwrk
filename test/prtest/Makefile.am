include_ipcdir=$(top_srcdir)/ipc
prtest_LDFLAGS=@TSTLDFLG@
prtest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES= libprsvr.la
libprsvr_la_SOURCES = prsvr.cpp prcli.cpp prsvr.h
libprsvr_la_LDFLAGS=-version-info @VERSION@
libprsvr_la_LIBADD= $(prtest_LDFLAGS) -lpthread -ldl
libprsvr_la_CPPFLAGS=$(prtest_CPPFLAGS)

prtest_PROGRAMS= prsvrtst prclitst
prtestdir=$(bindir)

prclitst_DATA= prdesc.json
prclitstdir=$(sysconfdir)/rpcf

prclitst_SOURCES= prtest.cpp prtest.h
prclitst_LDADD=$(prtest_LDFLAGS) libprsvr.la
prclitst_CPPFLAGS=$(prtest_CPPFLAGS) -DCLIENT

prsvrtst_SOURCES= prtest.cpp prtest.h
prsvrtst_LDADD=$(prtest_LDFLAGS) libprsvr.la
prsvrtst_CPPFLAGS=$(prtest_CPPFLAGS) -DSERVER

link_dir=./
