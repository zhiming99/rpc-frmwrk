rpcf_logger_PROGRAMS=rpcf_logger
rpcf_logger_SOURCES=mainsvr.cpp logsvrbase.cpp LogServicesvr.cpp logger.h LogServicesvr.h logsvrbase.h

rpcf_logger_LDADD=../ipc/libipc.la $(COMMON_LINK) \
 ../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party $(top_srcdir)/monitor/client/cpp/appmancli/libappmancli.la 

rpcf_loggerconfdir= $(sysconfdir)/rpcf
dist_rpcf_loggerconf_DATA= loggerdesc.json loggerdrv.json
rpcf_loggerdir=$(bindir)

include_ipcdir=$(top_srcdir)/ipc 

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif
EXTRA_DIST= logger.ridl

rpcf_logger_CPPFLAGS= -I$(top_srcdir)/include -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) -I$(top_srcdir)/monitor/client/cpp/appmancli

liblogcli_la_LTLIBRARIES=liblogcli.la
liblogcli_la_SOURCES = LogServicecli.cpp logger.h LogServicecli.h logclibase.h
liblogcli_la_CPPFLAGS= -I$(top_srcdir)/include -I$(include_ipcdir) -fPIC $(OFFSET_MACRO)
liblogcli_la_LIBADD= ../ipc/libipc.la ../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../3rd_party
liblogcli_la_LDFLAGS= -version-info @VERSION2@
liblogcli_ladir=$(libdir)

liblogcli_HEADERS = logger.h logclibase.h
liblogclidir=$(includedir)/rpcf/ipc

rpcf_logger_LDFLAGS=-rdynamic

rpcf_sendlog_PROGRAMS=rpcf_sendlog
rpcf_sendlog_SOURCES=maincli.cpp
rpcf_sendlog_LDADD=./liblogcli.la ../ipc/libipc.la $(COMMON_LINK) \
 ../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party
rpcf_sendlogdir=$(bindir)
rpcf_sendlog_CPPFLAGS= -I$(top_srcdir)/include -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) -I$(top_srcdir)/monitor/client/cpp/appmancli
rpcf_sendlog_LDFLAGS=-rdynamic
