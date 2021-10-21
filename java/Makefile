all:
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceP.java
	cpp -P -DJavaSerialImpl=JavaSerialHelperP -DGetIdHash=GetPeerIdHash -DInstType=CJavaProxyImpl JavaSerialImpl.java>org/rpcf/rpcbase/JavaSerialHelperP.java
	cpp -P -DJavaSerialImpl=JavaSerialHelperS -DGetIdHash=GetIdHashByChan -DInstType=CJavaServerImpl JavaSerialImpl.java>org/rpcf/rpcbase/JavaSerialHelperS.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir org/rpcf/rpcbase rpcbase.i
