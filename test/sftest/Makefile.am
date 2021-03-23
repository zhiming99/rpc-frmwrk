include_ipcdir=$(top_srcdir)/ipc
sftest_LDFLAGS=@TSTLDFLG@
sftest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES= libsfsvr.la
libsfsvr_la_SOURCES = sfsvr.cpp sfcli.cpp
libsfsvr_la_LDFLAGS=-version-info 0:0:0
libsfsvr_la_LIBADD= -lpthread -ldl
libsfsvr_la_CPPFLAGS=$(sftest_CPPFLAGS)

sftest_PROGRAMS= sfsvrtst sfclitst
sftestdir=$(bindir)

sfclitst_DATA= sfdesc.json
sfclitstdir=$(sysconfdir)/rpcf

sfclitst_SOURCES= sftest.cpp
sfclitst_LDADD=$(sftest_LDFLAGS) libsfsvr.la
sfclitst_CPPFLAGS=$(sftest_CPPFLAGS) -DCLIENT

sfsvrtst_SOURCES= sftest.cpp
sfsvrtst_LDADD=$(sftest_LDFLAGS) libsfsvr.la
sfsvrtst_CPPFLAGS=$(sftest_CPPFLAGS) -DSERVER

link_dir=./

