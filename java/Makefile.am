JDKDIR=$$(dirname $$(realpath `which javac`))/..
rpcbasedir=./src/org/rpcf/rpcbase
lib_LTLIBRARIES=librpcbaseJNI.la

nodist_librpcbaseJNI_la_SOURCES=rpcbase_wrap.cpp
librpcbaseJNI_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(JDKDIR)/include -I$(JDKDIR)/include/linux

librpcbaseJNI_ladir=$(libdir)
librpcbaseJNI_la_LDFLAGS = -version-info @VERSION2@
librpcbaseJNI_la_LIBADD = -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/3rd_party -lipc -lcombase -lpthread -ldl $(COMMON_LINK) 

javawrapper_DATA=rpcbase.jar
javawrapperdir=$(libdir)/rpcf

EXTRA_DIST = JavaRpcService.java $(rpcbasedir)/JRetVal.java $(rpcbasedir)/Helpers.java $(rpcbasedir)/JavaRpcContext.java $(rpcbasedir)/JavaReqContext.java $(rpcbasedir)/RC.java $(rpcbasedir)/JavaRpcServer.java $(rpcbasedir)/JavaRpcProxy.java $(rpcbasedir)/IRpcService.java rpcbase.i proxy.i server.i

rpcbase.jar: rpcbase_wrap.cpp
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl -DIsServer="isServer(){ return true;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl -DIsServer="isServer(){ return false;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceP.java
	javac src/org/rpcf/rpcbase/*.java
	cd src;jar -cf ../rpcbase.jar org/rpcf/rpcbase/*.class

rpcbase_wrap.cpp:rpcbase.i proxy.i server.i
	swig -c++ -java -package org.rpcf.rpcbase -outdir src/org/rpcf/rpcbase -o rpcbase_wrap.cpp rpcbase.i
