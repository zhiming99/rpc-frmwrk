JDKDIR=$$(dirname $$(realpath `which javac`))/..
rpcbasedir=./src/org/rpcf/rpcbase
testsdir=./src/org/rpcf/tests
lib_LTLIBRARIES=librpcbaseJNI.la

librpcbaseJNI_la_SOURCES = rpcbase_wrap.cpp
librpcbaseJNI_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(JDKDIR)/include -I$(JDKDIR)/include/linux

librpcbaseJNI_ladir=$(libdir)
librpcbaseJNI_la_LDFLAGS = -version-info @VERSION2@
librpcbaseJNI_la_LIBADD = -L../../combase -L.. -L../../ipc -L$(top_srcdir)/3rd_party -lipc -lcombase -lpthread -ldl $(COMMON_LINK) 

EXTRA_DIST = JavaRpcService.java $(rpcbasedir)/JRetVal.java $(rpcbasedir)/Helpers.java $(rpcbasedir)/JavaRpcContext.java $(rpcbasedir)/JavaReqContext.java $(rpcbasedir)/RC.java $(rpcbasedir)/JavaRpcServer.java $(rpcbasedir)/JavaRpcProxy.java\
 $(testsdir)/actcancel/ActiveCancelcli.java\
 $(testsdir)/actcancel/ActiveCancelsvr.java\
 $(testsdir)/actcancel/maincli.java\
 $(testsdir)/asynctst/AsyncTestcli.java\
 $(testsdir)/asynctst/AsyncTestsvr.java\
 $(testsdir)/asynctst/maincli.java\
 $(testsdir)/evtest/EventTestcli.java\
 $(testsdir)/evtest/maincli.java\
 $(testsdir)/evtest/mainsvr.java\
 $(testsdir)/iftest/IfTestsvr.java\
 $(testsdir)/iftest/maincli.java\
 $(testsdir)/katest/KeepAlivesvr.java\
 $(testsdir)/katest/maincli.java\
 $(testsdir)/stmtest/StreamTestcli.java\
 $(testsdir)/stmtest/StreamTestsvr.java\
 $(testsdir)/stmtest/TransferContext.java\
 $(testsdir)/stmtest/maincli.java\
 src/actcancel.ridl\
 src/asynctst.ridl\
 src/evtest.ridl\
 src/iftest.ridl\
 src/katest.ridl\
 src/stmtest.ridl


nodist_librpcbaseJNI_la_SOURCES=rpcbase_wrap.cpp

rpcbase_wrap.cpp:rpcbase.i proxy.i server.i
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl -DIsServer="isServer(){ return true;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl -DIsServer="isServer(){ return false;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceP.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir src/org/rpcf/rpcbase -o rpcbase_wrap.cpp rpcbase.i
	javac src/org/rpcf/rpcbase/*.java
