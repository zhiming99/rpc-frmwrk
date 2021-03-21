rpcrouter_PROGRAMS=rpcrouter
rpcrouter_SOURCES=routmain.cpp
rpcrouter_DATA=Readme.txt router.json rtauth.json
rpcrouter_LDADD=-lrpc -lipc -lcombase -lutils -lstdc++ -lcppunit \
-L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/rpc \
-L$(top_srcdir)/3rd_party

rpcrouter_HEADERS=routmain.h
rpcrouterdir=./

link_dir=$(top_srcdir)/combase/

include_rpcdir=$(top_srcdir)/rpc
include_ipcdir=$(top_srcdir)/ipc

rpcrouter_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir)
