lib_LTLIBRARIES=libsslpt.la

libsslpt_la_SOURCES = dllexp.cpp sslfido.cpp sslutils.cpp sslfido.h

libsslpt_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(SYSROOT)/usr/include
EXTRA_DIST=Readme.md README_cn.md

libsslpt_ladir=$(libdir)
libsslpt_la_LDFLAGS = -version-info @VERSION2@
libsslpt_la_LIBADD = -L../../combase -L.. -L../../ipc -L$(top_srcdir)/3rd_party\
 ../librpc.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK_SSL) 

