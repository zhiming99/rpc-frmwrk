all:
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl -DIsServer="isServer(){ return true;}" JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl -DIsServer="isServer(){ return false;}" JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceP.java
	cpp -P -DJavaSerialImpl=JavaSerialHelperP -DGetIdHash=GetPeerIdHash -DInstType=CJavaProxyImpl JavaSerialImpl.java>org/rpcf/rpcbase/JavaSerialHelperP.java
	cpp -P -DJavaSerialImpl=JavaSerialHelperS -DGetIdHash=GetIdHashByChan -DInstType=CJavaServerImpl JavaSerialImpl.java>org/rpcf/rpcbase/JavaSerialHelperS.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir org/rpcf/rpcbase rpcbase.i
	cpp -P -DCRpcServices=CJavaProxy org/rpcf/rpcbase/CJavaInterfBaseP.java -o CJavaInterfBaseP.java;mv CJavaInterfBaseP.java org/rpcf/rpcbase/
	cpp -P -DCRpcServices=CJavaServer org/rpcf/rpcbase/CJavaInterfBaseS.java -o CJavaInterfBaseS.java;mv CJavaInterfBaseS.java org/rpcf/rpcbase/
	g++ -c -D_USE_LIBEV -ggdb -fPIC rpcbase_wrap.cxx `pkg-config --cflags dbus-1` -I../include -I../ipc -I/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.302.b08-2.fc34.x86_64/include -I/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.302.b08-2.fc34.x86_64/include/linux
	g++ -shared rpcbase_wrap.o -o _rpcbase.so
