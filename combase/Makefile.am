lib_LTLIBRARIES=libcombase.la

libcombase_la_SOURCES = clsids.cpp \
    autoptr.cpp \
    buffer.cpp \
	config.cpp \
    objbase.cpp \
	registry.cpp \
	creatins.cpp \
	serial.cpp \
	dbuserr.cpp \
	objfctry.cpp \
	dmsgptr.cpp \
    seribase.cpp \
	variant.cpp

include_dir=$(top_srcdir)/include

rpcfcombase_HEADERS= $(include_dir)/autoptr.h \
    $(include_dir)/configdb.h \
    $(include_dir)/curclsid.h \
    $(include_dir)/dbuserr.h \
    $(include_dir)/defines.h \
    $(include_dir)/dmsgptr.h \
    $(include_dir)/nsdef.h \
    $(include_dir)/objfctry.h \
    $(include_dir)/propids.h \
    $(include_dir)/registry.h \
    $(include_dir)/rpc.h \
    $(include_dir)/seribase.h \
    $(include_dir)/stlcont.h \
    $(include_dir)/buffer.h \
    $(include_dir)/clsids.h \
    $(include_dir)/variant.h

rpcfcombasedir=$(includedir)/rpcf

libcombase_ladir=$(libdir)

libcombase_la_LDFLAGS=-version-info @VERSION2@
libcombase_la_LIBADD= -L../3rd_party -lutils -lpthread -ldl -llz4 $(COMMON_LINK)
