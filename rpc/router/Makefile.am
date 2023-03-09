rpcrouter_PROGRAMS=rpcrouter
rpcrouter_SOURCES=routmain.cpp routmain.h rtfiles.cpp rtfiles.h README.md rpcrouter.1
rpcrouter_LDADD=$(COMMON_LINK) ../librpc.la ../../fuse/libfuseif.la ../../ipc/libipc.la \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/rpc \
 -L$(top_srcdir)/3rd_party -L$(top_srcdir)/fuse 

man_MANS=rpcrouter.1

rpcrouterconfdir= $(sysconfdir)/rpcf
dist_rpcrouterconf_DATA=router.json rtauth.json
rpcrouterdir=$(bindir)

link_dir=$(top_srcdir)/combase/

include_rpcdir=$(top_srcdir)/rpc
include_ipcdir=$(top_srcdir)/ipc
include_fusedir=$(top_srcdir)/fuse

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

rpcrouter_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -fPIC \
 -I$(include_fusedir) $(OFFSET_MACRO)
rpcrouter_LDFLAGS=-rdynamic
