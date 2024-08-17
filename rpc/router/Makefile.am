rpcrouter_PROGRAMS=rpcrouter
rpcrouter_SOURCES=routmain.cpp routmain.h rpcrouter.1

rpcrouter_LDADD=
if ENABLE_FUSE3
rpcrouter_LDADD+=-L$(top_srcdir)/fuse ../../fuse/libfuseif.la librtfiles.la
endif

rpcrouter_LDADD+=../librpc.la ../../ipc/libipc.la $(COMMON_LINK) \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/rpc \
 -L$(top_srcdir)/3rd_party 

man_MANS=rpcrouter.1

rpcrouterconfdir= $(sysconfdir)/rpcf
dist_rpcrouterconf_DATA=router.json rtauth.json
rpcrouterdir=$(bindir)

link_dir=$(top_srcdir)/combase/

include_rpcdir=$(top_srcdir)/rpc
include_ipcdir=$(top_srcdir)/ipc

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

EXTRA_DIST=README.md README_cn.md
rpcrouter_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 

if ENABLE_FUSE3
include_fusedir=$(top_srcdir)/fuse
rpcrouter_CPPFLAGS+=-I$(include_fusedir) -DFUSE3

librtfiles_la_LTLIBRARIES=librtfiles.la
librtfiles_la_SOURCES = rtfiles.cpp rtfiles.h
librtfiles_la_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) -I$(include_fusedir) -DFUSE3
librtfiles_HEADERS=
librtfiles_la_LIBADD= ../../rpc/librpc.la ../../fuse/libfuseif.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../../3rd_party
librtfiles_la_LDFLAGS= -version-info @VERSION2@
librtfiles_ladir=$(libdir)/rpcf
librtfilesdir=$(includedir)/rpcf/ipc

endif

if ENABLE_AUTH
rpcrouter_CPPFLAGS+=-DAUTH
endif

rpcrouter_LDFLAGS=-rdynamic
