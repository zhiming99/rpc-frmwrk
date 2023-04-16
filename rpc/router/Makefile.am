rpcrouter_PROGRAMS=rpcrouter
rpcrouter_SOURCES=routmain.cpp routmain.h README.md rpcrouter.1
rpcrouter_LDADD=$(COMMON_LINK) ../librpc.la ../../ipc/libipc.la \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/rpc \
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

rpcrouter_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 

if FUSE3
include_fusedir=$(top_srcdir)/fuse
rpcrouter_LDADD+=-L$(top_srcdir)/fuse ../../fuse/libfuseif.la 
rpcrouter_CPPFLAGS+=-I$(include_fusedir) -DFUSE3
rpcrouter_SOURCES+=rtfiles.cpp rtfiles.h
endif

rpcrouter_LDFLAGS=-rdynamic
