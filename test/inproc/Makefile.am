include_ipcdir=$(top_srcdir)/ipc
inproc_LDFLAGS=@TSTLDFLG@
inproc_CPPFLAGS= -I$(include_ipcdir) -fPIC

libinproc_LTLIBRARIES=libinproc.la
libinprocdir=$(libdir)/rpcf
libinproc_la_SOURCES = inprocsvr.cpp inprocli.cpp inprocsvr.h
libinproc_la_LDFLAGS=-version-info @VERSION2@
libinproc_la_LIBADD= $(inproc_LDFLAGS) -lpthread -ldl
libinproc_la_CPPFLAGS=$(inproc_CPPFLAGS)

inproc_PROGRAMS=inproctst
inprocdir=$(bindir)/rpcf

dist_inproconf_DATA=inproc.json
inproconfdir=$(sysconfdir)/rpcf

inproctst_SOURCES= inproctst.cpp inproctst.h
inproctst_LDADD=$(inproc_LDFLAGS) libinproc.la -lpthread
inproctst_CPPFLAGS=$(inproc_CPPFLAGS)

link_dir=./
