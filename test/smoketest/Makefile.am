include_ipcdir=$(top_srcdir)/ipc
smktest_LDFLAGS=@TSTLDFLG@
smktest_CPPFLAGS= -I$(include_ipcdir) -fPIC

smktest_PROGRAMS=smoketest
smktestdir=$(bindir)/rpcf

smoketest_SOURCES= main.cpp
smoketest_LDADD=$(smktest_LDFLAGS)
smoketest_CPPFLAGS=$(smktest_CPPFLAGS)

link_dir=./

