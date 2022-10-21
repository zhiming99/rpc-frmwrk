lib_LTLIBRARIES=libfuseif.la

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

libfuseif_la_SOURCES = fuseif.cpp fuseif_ll.cpp serijson.cpp

libfuseif_la_CXXFLAGS=-I $(top_srcdir)/ipc $(OFFSET_MACRO)

rpcffuseif_HEADERS=fuseif.h fusedefs.h serijson.h

rpcffuseifdir=$(includedir)/rpcf/ipc

libfuseif_ladir=$(libdir)
EXTRA_DIST=fuse_i.h

EXTRA_DIST+=examples/cpp/actcancel/fccli.cpp \
examples/python/actcancel/fccli.py \
examples/python/actcancel/maincli.py \
examples/python/hellowld/maincli.py \
examples/python/hellowld/mainsvr.py \
examples/python/katest/maincli.py \
examples/python/katest/mainsvr.py \
examples/python/testypes/dumpStream.py \
examples/python/testypes/maincli.py \
examples/python/testypes/mainsvr.py

libfuseif_la_LDFLAGS=-version-info @VERSION2@
libfuseif_la_LIBADD= -lpthread -ldl -llz4 $(COMMON_LINK_FUSE3) -L../3rd_party -lutils 

include_ipcdir=$(top_srcdir)/ipc

hostsvr_PROGRAMS=hostsvr
hostsvr_SOURCES=rpcfshst.cpp
hostsvrdir=$(bindir)

hostsvr_LDADD=$(COMMON_LINK_FUSE3) libfuseif.la ../ipc/libipc.la ../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/3rd_party -L$(top_srcdir)/fuse 

hostsvr_CPPFLAGS= -I$(include_ipcdir) -fPIC -DSERVER $(OFFSET_MACRO)

hostcli_PROGRAMS=hostcli
hostcli_SOURCES=rpcfshst.cpp
hostclidir=$(bindir)

hostcli_LDADD=$(COMMON_LINK_FUSE3) libfuseif.la ../ipc/libipc.la ../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/3rd_party -L$(top_srcdir)/fuse 

hostcli_CPPFLAGS= -I$(include_ipcdir) -fPIC -DCLIENT $(OFFSET_MACRO)
