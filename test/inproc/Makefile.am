include_ipcdir=$(top_srcdir)/ipc
inproc_LDFLAGS=@TSTLDFLG@
inproc_CPPFLAGS= -I$(include_ipcdir)

lib_LTLIBRARIES=libinproc.la
libinproc_la_SOURCES = inprocsvr.cpp inprocli.cpp
libinproc_la_LDFLAGS=-version-info 0:0:0
libinproc_la_LIBADD= -lpthread -ldl
libinproc_la_CPPFLAGS=$(inproc_CPPFLAGS)

inproc_PROGRAMS=inproctst
inprocdir=$(bindir)

inproctst_SOURCES= inproctst.cpp
inproctst_LDADD=$(inproc_LDFLAGS) libinproc.la -lpthread
inproctst_CPPFLAGS=$(inproc_CPPFLAGS)

link_dir=./
