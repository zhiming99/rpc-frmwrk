include_ipcdir=$(top_srcdir)/ipc
btinrt_LDFLAGS=@TSTLDFLG@
btinrt_CPPFLAGS= -I$(include_ipcdir)

libbtinrt_LTLIBRARIES=libbtinrt.la
libbtinrtdir=$(libdir)/rpcf
libbtinrt_la_SOURCES = btinrtsvr.cpp btinrtcli.cpp btinrtsvr.h
libbtinrt_la_LDFLAGS=-version-info @VERSION2@
libbtinrt_la_LIBADD= $(btinrt_LDFLAGS) -lpthread -ldl
libbtinrt_la_CPPFLAGS=$(btinrt_CPPFLAGS)

btinrt_PROGRAMS=btinrtcli btinrtsvr
btinrtdir=$(bindir)/rpcf

dist_btinrtcli_DATA= btinrt.json
btinrtclidir=$(sysconfdir)/rpcf

btinrtcli_SOURCES= btinrtst.cpp btinrtst.h
btinrtcli_LDADD=$(btinrt_LDFLAGS) libbtinrt.la
btinrtcli_CPPFLAGS=$(btinrt_CPPFLAGS) -DCLIENT

btinrtsvr_SOURCES=btinrtst.cpp btinrtst.h
btinrtsvr_LDADD=$(btinrt_LDFLAGS) libbtinrt.la
btinrtsvr_CPPFLAGS=$(btinrt_CPPFLAGS) -DSERVER

link_dir=./
