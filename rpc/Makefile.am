lib_LTLIBRARIES=librpc.la

rpc_headers= \
conntask.h fdodrv.h prxyport.h stmrelay.h \
streammh.h tcportex.h tcpport.h rpcroute.h taskschd.h \
readme.md

librpc_la_SOURCES = \
taskschd.cpp ldbal.cpp dbusprxy.cpp prxyfdo.cpp fdodrv.cpp \
tcpport.cpp stmpdo.cpp tcpfido.cpp reqfwdr.cpp \
rpcroute.cpp bridge.cpp dllexp.cpp \
fidodrv.cpp stmrelay.cpp tcportex.cpp stmpdo2.cpp \
multihop.cpp streammh.cpp $(rpc_headers) 

include_ipcdir=$(top_srcdir)/ipc
librpc_la_CPPFLAGS = -I$(include_ipcdir)

rpcfrpcdir=$(top_srcdir)/rpc

librpc_la_LDFLAGS=-version-info @VERSION2@
librpc_la_LIBADD= -L../combase -L../ipc -lcombase -lipc -lpthread -ldl

SUBDIRS= . router

if ENABLE_OPENSSL
SUBDIRS += sslport wsport
endif

if ENABLE_KERBEROS
SUBDIRS += security
endif
