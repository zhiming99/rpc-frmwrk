lib_LTLIBRARIES=libfuseif.la

libfuseif_la_SOURCES = fuseif.cpp \
    fuseif_ll.cpp

libfuseif_la_CXXFLAGS=-I $(top_srcdir)/ipc

rpcffuseif_HEADERS=fuseif.h fusedefs.h

rpcffuseifdir=$(includedir)/rpcf/ipc

libfuseif_ladir=$(libdir)
EXTRA_DIST=fuse_i.h

libfuseif_la_LDFLAGS=-version-info @VERSION2@
libfuseif_la_LIBADD= -lpthread -ldl -llz4 $(COMMON_LINK_FUSE3) 
