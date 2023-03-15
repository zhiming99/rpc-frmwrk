lib_LTLIBRARIES=libgmsslpt.la
libgmsslpt_la_SOURCES = dllexp.cpp gmsfido.cpp agmsapi.cpp agmsapi.h gmsfido.h README_cn.md
libgmsslpt_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(SYSROOT)/usr/include -I/usr/local/include

libgmsslpt_ladir=$(libdir)
libgmsslpt_la_LDFLAGS = -version-info @VERSION2@
libgmsslpt_la_LIBADD = -L../../combase -L.. -L../../ipc -L$(top_srcdir)/3rd_party -L/usr/local/lib ../librpc.la ../../ipc/libipc.la ../../combase/libcombase.la -lgmssl -lpthread -ldl


