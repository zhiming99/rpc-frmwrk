lib_LTLIBRARIES=libfuseif.la

libfuseif_la_SOURCES = fuseif.cpp \
    fuseif_ll.cpp serijson.cpp

libfuseif_la_CXXFLAGS=-I $(top_srcdir)/ipc

rpcffuseif_HEADERS=fuseif.h fusedefs.h serijson.h

rpcffuseifdir=$(includedir)/rpcf/ipc

libfuseif_ladir=$(libdir)
EXTRA_DIST=fuse_i.h

EXTRA_DIST+=examples/cpp/actcancel/fccli.cpp \
examples/python/actcancel/fccli.py \
examples/python/actcancel/maincli.py \
examples/python/hellowld/maincli.py \
examples/python/hellowld/mainsvr.py \
examples/python/iolib.py \
examples/python/katest/maincli.py \
examples/python/katest/mainsvr.py \
examples/python/testypes/dumpStream.py \
examples/python/testypes/maincli.py \
examples/python/testypes/mainsvr.py

libfuseif_la_LDFLAGS=-version-info @VERSION2@
libfuseif_la_LIBADD= -lpthread -ldl -llz4 $(COMMON_LINK_FUSE3) -L../3rd_party -lutils 
