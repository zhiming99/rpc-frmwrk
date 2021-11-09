all:
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl -DIsServer="isServer(){ return true;}" JavaRpcService.java>src/org/rpcf/rpcbase/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl -DIsServer="isServer(){ return false;}" JavaRpcService.java>src/org/rpcf/rpcbase/JavaRpcServiceP.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir src/org/rpcf/rpcbase rpcbase.i
	g++ -c -D_USE_LIBEV -ggdb -fPIC rpcbase_wrap.cxx `pkg-config --cflags dbus-1` -I../include -I../ipc -I/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.312.b07-1.fc34.x86_64/include -I/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.312.b07-1.fc34.x86_64/include/linux
	g++ -shared rpcbase_wrap.o -lipc -lcombase -L../ipc/.libs/ -L../combase/.libs/ -Wl,-rpath=../ipc/.libs -Wl,-rpath=../combase/.libs -o librpcbaseJNI.so
	javac src/org/rpcf/rpcbase/*.java
