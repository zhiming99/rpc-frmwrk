link_dir=$(top_srcdir)/combase/
include_ipcdir=$(top_srcdir)/ipc

regfsmnt_PROGRAMS=regfsmnt
regfsmnt_SOURCES=regfuse.cpp

regfsmnt_LDADD=libregfs.la

regfsmnt_LDADD+=../../ipc/libipc.la \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party 

man_MANS=regfsmnt.1
regfsmnt_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 

regfsmntconfdir= $(sysconfdir)/rpcf
regfsmntdir=$(bindir)

if ENABLE_FUSE3
regfsmnt_CPPFLAGS+= -DFUSE3
regfsmnt_LDADD+= $(COMMON_LINK_FUSE3)
if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

else
regfsmnt_LDADD+= $(COMMON_LINK)
endif
regfsmnt_LDFLAGS=-rdynamic

EXTRA_DIST=README_cn.md regfsif.ridl regfs.ridl regfsmnt.1

libregfs_la_LTLIBRARIES=libregfs.la
libregfs_la_SOURCES = regfs.cpp blkalloc.h blkalloc.cpp dir.cpp file.cpp \
dllexp.cpp RegistryFs.cpp RegistryFs.h RegFsSvcLocalsvr.h RegFsSvcLocalsvr.cpp
libregfs_la_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO)
libregfs_HEADERS=
libregfs_la_LIBADD= ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../../3rd_party
libregfs_la_LDFLAGS= -version-info @VERSION2@
libregfs_ladir=$(libdir)
libregfsdir=$(includedir)/rpcf/ipc

regfschk_PROGRAMS=regfschk
regfschk_SOURCES=regfschk.cpp

regfschk_LDADD=libregfs.la

regfschk_LDADD+=../../ipc/libipc.la \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party 

regfschk_CPPFLAGS= -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 

regfschkconfdir= $(sysconfdir)/rpcf
regfschkdir=$(bindir)

regfschk_LDADD+= $(COMMON_LINK)
regfschk_LDFLAGS=-rdynamic

