lib_LTLIBRARIES=librpc.la

rpc_headers= \
conntask.h stmrelay.h \
streammh.h tcportex.h tcpport.h rpcroute.h taskschd.h \
readme.md

librpc_la_SOURCES = \
taskschd.cpp ldbal.cpp \
tcpport.cpp stmpdo.cpp tcpfido.cpp reqfwdr.cpp \
rpcroute.cpp bridge.cpp dllexp.cpp \
fidodrv.cpp stmrelay.cpp tcportex.cpp stmpdo2.cpp \
multihop.cpp streammh.cpp $(rpc_headers) 

include_ipcdir=$(top_srcdir)/ipc
librpc_la_CPPFLAGS = -I$(include_ipcdir)

rpcfrpcdir=$(top_srcdir)/rpc

librpc_la_LDFLAGS=-version-info @VERSION2@
librpc_la_LIBADD= -L../combase -L../ipc ../combase/libcombase.la ../ipc/libipc.la -lpthread -ldl

#authprxy.json is required by tools/rpcfg.py
#put it here so that disabling security module
#will not affect its installation
dist_rpcconf_DATA=security/authprxy.json
rpcconfdir=$(sysconfdir)/rpcf

SUBDIRS= . router

if ENABLE_GMSSL
SUBDIRS += gmsslport
endif

if ENABLE_OPENSSL
SUBDIRS += sslport wsport
else

if ENABLE_GMSSL
SUBDIRS += wsport
endif

endif

if ENABLE_AUTH
SUBDIRS += security
endif
