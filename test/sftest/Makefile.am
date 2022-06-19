include_ipcdir=$(top_srcdir)/ipc
sftest_LDFLAGS=@TSTLDFLG@ -lpthread 
sftest_CPPFLAGS= -I$(include_ipcdir) -fPIC

libsfsvr_LTLIBRARIES= libsfsvr.la
libsfsvrdir=$(libdir)/rpcf
libsfsvr_la_SOURCES = sfsvr.cpp sfcli.cpp sfsvr.h
libsfsvr_la_LDFLAGS=-version-info @VERSION2@
libsfsvr_la_LIBADD= $(sftest_LDFLAGS) -ldl
libsfsvr_la_CPPFLAGS=$(sftest_CPPFLAGS)

sftest_PROGRAMS= sfsvrtst sfclitst
sftestdir=$(bindir)/rpcf

dist_sfclitst_DATA= sfdesc.json
sfclitstdir=$(sysconfdir)/rpcf

sfclitst_SOURCES= sftest.cpp sftest.h
sfclitst_LDADD=$(sftest_LDFLAGS) libsfsvr.la
sfclitst_CPPFLAGS=$(sftest_CPPFLAGS) -DCLIENT

sfsvrtst_SOURCES= sftest.cpp Readme.txt
sfsvrtst_LDADD=$(sftest_LDFLAGS) libsfsvr.la
sfsvrtst_CPPFLAGS=$(sftest_CPPFLAGS) -DSERVER

link_dir=./

