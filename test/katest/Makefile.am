include_ipcdir=$(top_srcdir)/ipc
katest_LDFLAGS=@TSTLDFLG@
katest_CPPFLAGS= -I$(include_ipcdir)

libkasvr_LTLIBRARIES=libkasvr.la
libkasvrdir=$(libdir)/rpcf
libkasvr_la_SOURCES = kasvr.cpp kacli.cpp kasvr.h
libkasvr_la_LDFLAGS=-version-info @VERSION2@
libkasvr_la_LIBADD= $(katest_LDFLAGS) -lpthread -ldl
libkasvr_la_CPPFLAGS=$(katest_CPPFLAGS)

katest_PROGRAMS=kasvrtst kaclitst
katestdir=$(bindir)/rpcf

dist_kaclitst_DATA=kadesc.json
kaclitstdir=$(sysconfdir)/rpcf

kaclitst_SOURCES= katest.cpp katest.h
kaclitst_LDADD=$(katest_LDFLAGS) libkasvr.la
kaclitst_CPPFLAGS=$(katest_CPPFLAGS) -DCLIENT

kasvrtst_SOURCES= katest.cpp katest.h
kasvrtst_LDADD=$(katest_LDFLAGS) libkasvr.la
kasvrtst_CPPFLAGS=$(katest_CPPFLAGS) -DSERVER

link_dir=./
