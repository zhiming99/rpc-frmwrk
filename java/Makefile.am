JDKDIR=$$(dirname $$(realpath `which javac`))/..
rpcbasedir=./src/org/rpcf/rpcbase
lib_LTLIBRARIES=librpcbaseJNI.la

if ENABLE_FUSE3
fuse3_macro=-DFUSE3
fuse3_lib=$(top_srcdir)/fuse/libfuseif.la
fuse3_inc=-I$(top_srcdir)/fuse
fuse3_src=fuseif.i
else
fuse3_macro=
fuse3_lib=
fuse3_inc=
fuse3_src=
endif

nodist_librpcbaseJNI_la_SOURCES=rpcbase_wrap.cpp
librpcbaseJNI_la_CPPFLAGS = -I$(top_srcdir)/ipc -I../ -I$(top_srcdir)/combase -I$(JDKDIR)/include -I$(JDKDIR)/include/linux $(fuse3_macro) $(fuse3_inc)

librpcbaseJNI_ladir=$(libdir)
librpcbaseJNI_la_LDFLAGS = -version-info @VERSION2@
librpcbaseJNI_la_LIBADD = -L$(top_srcdir)/combase -L$(top_srcdir)/ipc -L$(top_srcdir)/3rd_party $(fuse3_lib) ../ipc/libipc.la ../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) 

javawrapper_DATA=rpcbase.jar
javawrapperdir=$(libdir)/rpcf

rpcbasejava = JavaRpcService.java $(rpcbasedir)/JRetVal.java $(rpcbasedir)/Helpers.java $(rpcbasedir)/JavaRpcContext.java $(rpcbasedir)/JavaReqContext.java $(rpcbasedir)/RC.java $(rpcbasedir)/JavaRpcServer.java $(rpcbasedir)/JavaRpcProxy.java $(rpcbasedir)/IRpcService.java
EXTRA_DIST = $(rpcbasejava) rpcbase.i proxy.i server.i fastrpc.i fuseif.i

rpcbase.jar: rpcbase_wrap.cpp $(rpcbasejava) Makefile.am
	cpp -P -DJavaRpcService=JavaRpcServiceS -DInstType=CJavaServer -DIsServer="isServer(){ return true;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceS.java
	cpp -P -DJavaRpcService=JavaRpcServiceP -DInstType=CJavaProxy -DIsServer="isServer(){ return false;}" JavaRpcService.java -o $(rpcbasedir)/JavaRpcServiceP.java
	javac src/org/rpcf/rpcbase/*.java
	cd src;jar -cf ../rpcbase.jar org/rpcf/rpcbase/*.class

rpcbase_wrap.cpp:rpcbase.i proxy.i server.i fastrpc.i $(fuse3_src)
	swig -c++ -java $(fuse3_macro) -package org.rpcf.rpcbase -outdir src/org/rpcf/rpcbase -o rpcbase_wrap.cpp rpcbase.i
