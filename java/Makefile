all:
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServerImpl JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxyImpl JavaRpcService.java>org/rpcf/rpcbase/JavaRpcServiceP.java
	swig -c++ -java -package org.rpcf.rpcbase -outdir org/rpcf/rpcbase rpcbase.i
