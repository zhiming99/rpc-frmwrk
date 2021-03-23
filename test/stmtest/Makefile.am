include_ipcdir=$(top_srcdir)/ipc
stmtest_LDFLAGS=@TSTLDFLG@
stmtest_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libstmsvr.la
libstmsvr_la_SOURCES = stmsvr.cpp stmcli.cpp
libstmsvr_la_LDFLAGS=-version-info 0:0:0
libstmsvr_la_LIBADD= -lpthread -ldl
libstmsvr_la_CPPFLAGS=$(stmtest_CPPFLAGS)

stmtest_PROGRAMS=stmsvrtst stmclitst
stmtestdir=$(bindir)

stmclitst_DATA=stmdesc.json
stmclitstdir=$(sysconfdir)/rpcf

stmclitst_SOURCES= stmtest.cpp
stmclitst_LDADD=$(stmtest_LDFLAGS) libstmsvr.la
stmclitst_CPPFLAGS=$(stmtest_CPPFLAGS) -DCLIENT

stmsvrtst_SOURCES=stmtest.cpp
stmsvrtst_LDADD=$(stmtest_LDFLAGS) libstmsvr.la
stmsvrtst_CPPFLAGS=$(stmtest_CPPFLAGS) -DSERVER

link_dir=./

