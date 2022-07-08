lib_LTLIBRARIES=libwspt.la

libwspt_la_SOURCES = dllexp.cpp wsfido.cpp wsfido.h Readme.md

libwspt_la_CPPFLAGS  = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase

libwspt_ladir=$(libdir)

libwspt_la_LDFLAGS = -version-info @VERSION2@
libwspt_la_LIBADD = $(COMMON_LINK) -L../../combase -L.. -L../../ipc -L./wsproto \
-L../../3rd_party -lwsock -lutils ../librpc.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl


SUBDIRS= wsproto
