lib_LTLIBRARIES=libipc.la

libipc_la_SOURCES = \
loopool.cpp stream.cpp portex.cpp iftasks.cpp ifstat.cpp \
rpcif.cpp sevpoll.cpp counters.cpp \
connmgr.cpp filetran.cpp mainloop.cpp dllexp.cpp \
evloop.cpp dbushook.cpp irp.cpp irpthread.cpp \
port.cpp portstat.cpp drvmgr.cpp utils.cpp \
iomgr.cpp ptifmap.cpp pnpmgr.cpp dbusdrv.cpp \
dbusport.cpp portdrv.cpp rpcpdo.cpp dbuspdo.cpp \
tasklets.cpp lpbkmtch.cpp ifhelper.cpp iiddict.cpp \
reqopen.cpp uxport.cpp uxstream.cpp streamex.cpp \
prxyfdo.cpp dbusprxy.cpp fdodrv.cpp fastrpc.cpp \
stmport.cpp msgmatch.cpp stmcp.cpp tractgrp.cpp

include_dir=$(top_srcdir)/include

rpcfipc_HEADERS=  \
loopool.h connhelp.h counters.h dbusport.h emaphelp.h \
evloop.h filetran.h frmwrk.h ifhelper.h \
ifstat.h iftasks.h iiddict.h irp.h jsondef.h \
mainloop.h msgmatch.h namespc.h port.h \
portdrv.h portex.h reqopen.h sevpoll.h \
stream.h streamex.h tasklets.h tractgrp.h \
utils.h uxport.h uxstream.h proxy.h prxyport.h fdodrv.h \
stmport.h fastrpc.h stmcp.h seqtgmgr.h

rpcfipcdir=$(includedir)/rpcf/ipc

dist_ipcconf_DATA=driver.json
ipcconfdir=$(sysconfdir)/rpcf

libipc_ladir=$(libdir)
libipc_la_CPPFLAGS= -I$(top_srcdir)/logger

libipc_la_LDFLAGS=-version-info @VERSION2@
libipc_la_LIBADD= -L../combase ../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) 
