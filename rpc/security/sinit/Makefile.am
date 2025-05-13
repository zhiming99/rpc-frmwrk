mon_dir=$(top_srcdir)/monitor

sinit_PROGRAMS=sinit
sinit_SOURCES=sinit.cpp encdec.cpp encdecg.cpp
sinit_LDADD= $(mon_dir)/registry/libregfs.la $(top_srcdir)/ipc/libipc.la \
$(top_srcdir)/combase/libcombase.la -lutils -lstdc++ -lpthread -ldl \
 -L$(top_srcdir)/3rd_party -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc
if ENABLE_OPENSSL
sinit_LDADD+= $(COMMON_LINK_SSL) 
endif
if ENABLE_GMSSL
sinit_LDADD+= -L/usr/local/lib -lgmssl 
endif

sinit_CPPFLAGS= -I. -I$(mon_dir)/registry -I$(top_srcdir)/include \
 -I$(top_srcdir)/ipc -fPIC $(OFFSET_MACRO)
if ENABLE_OPENSSL
sinit_CPPFLAGS+= -DOPENSSL
endif
if ENABLE_GMSSL
sinit_CPPFLAGS+= -DGMSSL
endif



sinit_LDFLAGS=-rdynamic
sinitdir=$(bindir)/rpcf
