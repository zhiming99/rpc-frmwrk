JDKDIR=$$(dirname $$(realpath `which javac`))/../
all:
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl -DIsServer="isServer(){ return true;}" JavaRpcService.java>src/org/rpcf/rpcbase/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl -DIsServer="isServer(){ return false;}" JavaRpcService.java>src/org/rpcf/rpcbase/JavaRpcServiceP.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir src/org/rpcf/rpcbase -o rpcbase_wrap.cpp rpcbase.i
	g++ -c -D_USE_LIBEV -ggdb -fPIC rpcbase_wrap.cpp `pkg-config --cflags dbus-1` -I../include -I../ipc -I${JDKDIR}include -I${JDKDIR}include/linux
	g++ -shared rpcbase_wrap.o -lipc -lcombase -L../ipc/.libs/ -L../combase/.libs/ -Wl,-rpath=../ipc/.libs -Wl,-rpath=../combase/.libs -o librpcbaseJNI.so
	javac src/org/rpcf/rpcbase/*.java
