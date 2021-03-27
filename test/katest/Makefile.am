include_ipcdir=$(top_srcdir)/ipc
katest_LDFLAGS=@TSTLDFLG@
katest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libkasvr.la
libkasvr_la_SOURCES = kasvr.cpp kacli.cpp kasvr.h
libkasvr_la_LDFLAGS=-version-info 0:0:0
libkasvr_la_LIBADD= $(katest_LDFLAGS) -lpthread -ldl
libkasvr_la_CPPFLAGS=$(katest_CPPFLAGS)

katest_PROGRAMS=kasvrtst kaclitst
katestdir=$(bindir)

kaclitst_DATA=kadesc.json
kaclitstdir=$(sysconfdir)/rpcf

kaclitst_SOURCES= katest.cpp katest.h
kaclitst_LDADD=$(katest_LDFLAGS) libkasvr.la
kaclitst_CPPFLAGS=$(katest_CPPFLAGS) -DCLIENT

kasvrtst_SOURCES= katest.cpp katest.h
kasvrtst_LDADD=$(katest_LDFLAGS) libkasvr.la
kasvrtst_CPPFLAGS=$(katest_CPPFLAGS) -DSERVER

link_dir=./
