rpcrouter_PROGRAMS=rpcrouter
rpcrouter_SOURCES=routmain.cpp routmain.h README.md rpcrouter.1
rpcrouter_LDADD=$(COMMON_LINK) -lrpc -lipc -lcombase -lutils -lstdc++ -lcppunit \
-L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/rpc \
-L$(top_srcdir)/3rd_party

man_MANS=rpcrouter.1

rpcrouterconfdir= $(sysconfdir)/rpcf
dist_rpcrouterconf_DATA=router.json rtauth.json
rpcrouterdir=$(bindir)

link_dir=$(top_srcdir)/combase/

include_rpcdir=$(top_srcdir)/rpc
include_ipcdir=$(top_srcdir)/ipc

rpcrouter_CPPFLAGS= -I$(include_rpcdir) -I$(include_ipcdir) -fPIC
