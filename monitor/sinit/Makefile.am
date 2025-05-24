mon_dir=$(top_srcdir)/monitor

sinit_PROGRAMS=sinit
sinit_SOURCES=sinit.cpp
sinit_LDADD=librpcfenc.la $(mon_dir)/registry/libregfs.la $(top_srcdir)/ipc/libipc.la \
$(top_srcdir)/combase/libcombase.la -lutils -lstdc++ -lpthread -ldl \
 -L$(top_srcdir)/3rd_party -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc

sinit_CPPFLAGS= -I. -I$(mon_dir)/registry -I$(top_srcdir)/include \
 -I$(top_srcdir)/ipc -fPIC $(OFFSET_MACRO)
sinit.cpp: $(lib_LTLIBRARIES)


sinit_LDFLAGS=-rdynamic -version-info @VERSION2@
sinitdir=$(bindir)/rpcf

lib_LTLIBRARIES=librpcfenc.la
librpcfenc_la_SOURCES= encdec.cpp encdecg.cpp encdec.h
librpcfenc_la_LIBADD=
librpcfenc_la_CPPFLAGS= -I. -I$(mon_dir)/registry -I$(top_srcdir)/include \
 -I$(top_srcdir)/ipc -fPIC $(OFFSET_MACRO)
librpcfenc_la_LDFLAGS= -rdynamic -version-info @VERSION2@

if ENABLE_OPENSSL
librpcfenc_la_LIBADD+= $(COMMON_LINK_SSL) 
librpcfenc_la_CPPFLAGS += -DOPENSSL
endif

if ENABLE_GMSSL
librpcfenc_la_LIBADD+= -L/usr/local/lib -lgmssl 
librpcfenc_la_CPPFLAGS+= -DGMSSL
endif

