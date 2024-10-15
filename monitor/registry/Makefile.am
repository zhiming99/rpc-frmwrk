if ENABLE_FUSE3
rfmount_PROGRAMS=rfmount
rfmount_SOURCES=regfuse.cpp

rfmount_LDADD=
rfmount_LDADD+=-L$(top_srcdir)/fuse ../../fuse/libfuseif.la libregfs.la

rfmount_LDADD+=../../ipc/libipc.la $(COMMON_LINK) \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party 

#man_MANS=rfmount.1

rfmountconfdir= $(sysconfdir)/rpcf
rfmountdir=$(bindir)

link_dir=$(top_srcdir)/combase/

include_ipcdir=$(top_srcdir)/ipc

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

rfmount_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 

include_fusedir=$(top_srcdir)/fuse
rfmount_CPPFLAGS+=-I$(include_fusedir) -DFUSE3
endif

#EXTRA_DIST=README.md README_cn.md

libregfs_la_LTLIBRARIES=libregfs.la
libregfs_la_SOURCES = regfs.cpp blkalloc.h blkalloc.cpp dir.cpp file.cpp dllexp.cpp
libregfs_la_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO)
libregfs_HEADERS=
libregfs_la_LIBADD= ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../../3rd_party
libregfs_la_LDFLAGS= -version-info @VERSION2@
libregfs_ladir=$(libdir)/rpcf
libregfsdir=$(includedir)/rpcf/ipc

rfmount_LDFLAGS=-rdynamic
