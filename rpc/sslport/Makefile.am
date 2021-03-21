lib_LTLIBRARIES=libsslpt.la

libsslpt_la_SOURCES = dllexp.cpp sslfido.cpp sslutils.cpp

libsslpt_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase

libsslpt_la_HEADERS = sslfido.h

dist_libsslpt_la_DATA=Readme.md

libsslpt_ladir=./

libsslpt_la_LDFLAGS = -version-info 0:0:0
libsslpt_la_LIBADD = -L../../combase -L.. -L../../ipc -lrpc -lipc -lcombase -lpthread -ldl

