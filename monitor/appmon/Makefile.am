link_dir=$(top_srcdir)/combase/
include_ipcdir=$(top_srcdir)/ipc

appmon_PROGRAMS=appmonsvr appmoncli
appmonsvr_SOURCES=mainsvr.cpp AppMonitorsvr.cpp AppMonitorsvr.h
if ENABLE_FUSE3
appmonsvr_SOURCES+=monfuse.cpp
endif
appmoncli_SOURCES=maincli.cpp AppMonitorcli.cpp AppMonitorcli.h

appmonsvr_LDADD= ./libappmon.a \
../registry/libregfs.la ../../ipc/libipc.la $(COMMON_LINK_FUSE3) \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party 

appmoncli_LDADD=$(appmonsvr_LDADD)

appmonconfdir=$(sysconfdir)/rpcf
appmondir=$(bindir)

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

appmonsvr_CPPFLAGS= -I../registry -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 
appmonsvr_LDFLAGS=-rdynamic

if ENABLE_FUSE3
appmonsvr_CPPFLAGS+= -DFUSE3
endif

appmoncli_CPPFLAGS=$(appmonsvr_CPPFLAGS)
appmoncli_LDFLAGS=$(appmonsvr_LDFLAGS)

EXTRA_DIST=appmon.ridl appmondesc.json

lib_LIBRARIES=libappmon.a
libappmon_a_SOURCES = appmon.cpp appmon.h 
libappmon_a_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO)
libappmon_a_HEADERS=
#libappmon_la_LIBADD= ../registry/libregfs.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../../3rd_party
#libappmon_la_LDFLAGS= -version-info @VERSION2@
#libappmon_ladir=$(libdir)
libappmon_adir=./

