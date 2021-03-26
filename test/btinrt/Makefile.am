include_ipcdir=$(top_srcdir)/ipc
btinrt_LDFLAGS=@TSTLDFLG@
btinrt_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libbtinrt.la
libbtinrt_la_SOURCES = btinrtsvr.cpp btinrtcli.cpp btinrtsvr.h
libbtinrt_la_LDFLAGS=-version-info 0:0:0
libbtinrt_la_LIBADD= -lpthread -ldl
libbtinrt_la_CPPFLAGS=$(btinrt_CPPFLAGS)

btinrt_PROGRAMS=btinrtcli btinrtsvr
btinrtdir=$(bindir)

btinrtcli_DATA= btinrt.json
btinrtclidir=$(sysconfdir)/rpcf

btinrtcli_SOURCES= btinrtst.cpp btinrtst.h
btinrtcli_LDADD=$(btinrt_LDFLAGS) libbtinrt.la
btinrtcli_CPPFLAGS=$(btinrt_CPPFLAGS) -DCLIENT

btinrtsvr_SOURCES=btinrtst.cpp btinrtst.h
btinrtsvr_LDADD=$(btinrt_LDFLAGS) libbtinrt.la
btinrtsvr_CPPFLAGS=$(btinrt_CPPFLAGS) -DSERVER

link_dir=./
