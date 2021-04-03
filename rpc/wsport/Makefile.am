lib_LTLIBRARIES=libwspt.la

libwspt_la_SOURCES = dllexp.cpp wsfido.cpp wsfido.h Readme.md

libwspt_la_CPPFLAGS  = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase

libwspt_ladir=$(libdir)

libwspt_la_LDFLAGS = -version-info @VERSION@
libwspt_la_LIBADD = -L../../combase -L.. -L../../ipc -L./wsproto \
-L../../3rd_party -lutils -lrpc -lipc -lcombase -lwsock -lpthread -ldl

SUBDIRS= wsproto
