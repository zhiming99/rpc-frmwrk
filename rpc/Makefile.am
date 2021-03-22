lib_LTLIBRARIES=librpc.la

librpc_la_SOURCES = \
ldbal.cpp dbusprxy.cpp prxyfdo.cpp fdodrv.cpp \
tcpport.cpp stmpdo.cpp tcpfido.cpp reqfwdr.cpp \
rpcroute.cpp bridge.cpp dllexp.cpp \
fidodrv.cpp stmrelay.cpp tcportex.cpp stmpdo2.cpp \
multihop.cpp streammh.cpp

include_ipcdir=$(top_srcdir)/ipc
librpc_la_CPPFLAGS = -I$(include_ipcdir)

#librpc_la_HEADERS=  \
#conntask.h fdodrv.h prxyport.h stmrelay.h \
#streammh.h tcportex.h tcpport.h

#librpc_la_DATA=readme.md

librpc_ladir=$(libdir)

librpc_la_LDFLAGS=-version-info 0:0:0
librpc_la_LIBADD= -L../combase -L../ipc -lcombase -lipc -lpthread -ldl

SUBDIRS= . router security sslport wsport

