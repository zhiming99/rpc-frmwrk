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

EXTRA_DIST+= \
examples/python/actcancel/ActiveCancelcli.py \
examples/python/actcancel/ActiveCancelsvr.py \
examples/python/actcancel/maincli.py \
examples/python/evtest/EventTestcli.py \
examples/python/evtest/maincli.py \
examples/python/evtest/mainsvr.py \
examples/python/hellowld/HelloWorldSvcsvr.py \
examples/python/hellowld/maincli.py \
examples/python/iftest/IfTestsvr.py \
examples/python/iftest/maincli.py \
examples/python/katest/KeepAlivecli.py \
examples/python/katest/KeepAlivesvr.py \
examples/python/katest/maincli.py \
examples/python/testypes/dumpStream.py \
examples/python/testypes/maincli.py \
examples/python/testypes/mainsvr.py \
examples/python/testypes/sendStream.py \
examples/python/testypes/tailStream.py \
examples/python/testypes2/TestTypesSvccli.py \
examples/python/testypes2/TestTypesSvcsvr.py \
examples/python/testypes2/maincli.py \
examples/python/testypes2/mainsvr.py

libfuseif_la_LDFLAGS=-version-info @VERSION2@
libfuseif_la_LIBADD= ../ipc/libipc.la ../combase/libcombase.la -lstdc++ $(COMMON_LINK_FUSE3) -lpthread -ldl -llz4 -L../3rd_party -lutils 

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
