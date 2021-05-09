include_ipcdir=$(top_srcdir)/ipc
stmtest_LDFLAGS=@TSTLDFLG@ 
stmtest_CPPFLAGS= -I$(include_ipcdir)

libstmsvr_LTLIBRARIES=libstmsvr.la
libstmsvrdir=$(libdir)/rpcf
libstmsvr_la_SOURCES = stmsvr.cpp stmcli.cpp stmsvr.h
libstmsvr_la_LDFLAGS=-version-info @VERSION2@
libstmsvr_la_LIBADD= $(stmtest_LDFLAGS) -lpthread -ldl
libstmsvr_la_CPPFLAGS=$(stmtest_CPPFLAGS)

stmtest_PROGRAMS=stmsvrtst stmclitst
stmtestdir=$(bindir)/rpcf

dist_stmclitst_DATA=stmdesc.json
stmclitstdir=$(sysconfdir)/rpcf
ext_linkopt=-Wl,-rpath -Wl,$(libdir)/rpcf
stmclitst_SOURCES= stmtest.cpp stmtest.h
stmclitst_LDADD=$(stmtest_LDFLAGS) libstmsvr.la 
stmclitst_CPPFLAGS=$(stmtest_CPPFLAGS) -DCLIENT

stmsvrtst_SOURCES=stmtest.cpp Readme.txt
stmsvrtst_LDADD=$(stmtest_LDFLAGS) libstmsvr.la
stmsvrtst_CPPFLAGS=$(stmtest_CPPFLAGS) -DSERVER

link_dir=./

