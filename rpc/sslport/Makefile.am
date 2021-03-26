lib_LTLIBRARIES=libsslpt.la

libsslpt_la_SOURCES = dllexp.cpp sslfido.cpp sslutils.cpp sslfido.h Readme.md

libsslpt_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(SYSROOT)/usr/include



libsslpt_ladir=$(libdir)
libsslpt_la_LDFLAGS = -version-info 0:0:0
libsslpt_la_LIBADD = $(COMMON_LINK) -L../../combase -L.. -L../../ipc -L$(top_srcdir)/3rd_party -lrpc -lipc -lcombase -lpthread -ldl

