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
    seribase.cpp 

include_dir=$(top_srcdir)/include

#rpcfcombase_HEADERS= $(include_dir)/autoptr.h \
#    $(include_dir)/configdb.h \
#    $(include_dir)/curclsid.h \
#    $(include_dir)/dbuserr.h \
#    $(include_dir)/defines.h \
#    $(include_dir)/dmsgptr.h \
#    $(include_dir)/nsdef.h \
#    $(include_dir)/objfctry.h \
#    $(include_dir)/propids.h \
#    $(include_dir)/registry.h \
#    $(include_dir)/rpc.h \
#    $(include_dir)/seribase.h \
#    $(include_dir)/stlcont.h \
#    $(include_dir)/buffer.h \
#    $(include_dir)/clsids.h

rpcfcombasedir=$(includedir)/rpcf/include

libcombase_ladir=$(libdir)

libcombase_la_LDFLAGS=-version-info 0:0:0
libcombase_la_LIBADD= -lpthread -ldl
