lib_LTLIBRARIES=libwspt.la

libwspt_la_SOURCES = dllexp.cpp wsfido.cpp

libwspt_la_CPPFLAGS  = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase

#libwspt_la_HEADERS = wsfido.h
#dist_libwspt_la_DATA=Readme.md

libwspt_ladir=$(libdir)

libwspt_la_LDFLAGS = -version-info 0:0:0
libwspt_la_LIBADD = -L../../combase -L.. -L../../ipc -L./wsproto \
-L../../3rd_party -lutils -lrpc -lipc -lcombase -lwsock -lpthread -ldl

SUBDIRS= wsproto
