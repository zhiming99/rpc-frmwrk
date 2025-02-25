link_dir=$(top_srcdir)/combase/
include_dir=$(top_srcdir)/ipc

lib_LTLIBRARIES=libappmoncli.la libappmancli.la
libappmoncli_la_SOURCES= maincli.cpp AppMonitorcli.cpp 
libappmancli_la_SOURCES= maincli2.cpp AppManagercli.cpp SimpleAuthcli.cpp

#libappmoncli_la_LDADD= ./libappmoncli.a \
# ../../registry/libregfs.la ../../../ipc/libipc.la $(COMMON_LINK_FUSE3) \
# ../../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
# -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
# -L$(top_srcdir)/3rd_party 

#appmoncli_LDADD=$(appmonsvr_LDADD)

appmoncliconf_DATA=
appmoncliconfdir=$(sysconfdir)/rpcf
appmonclidir=$(bindir)

libappmoncli_la_CPPFLAGS= -I. -I../../registry -I../../appmon -I$(top_srcdir)/include -I$(include_dir) -fPIC $(OFFSET_MACRO) 
libappmancli_la_CPPFLAGS=$(libappmoncli_la_CPPFLAGS)

EXTRA_DIST=

noinst_LIBRARIES=libappmoncli.a
libappmoncli_a_SOURCES = appmon.cpp appmonstruct.cpp IAppStorecli.cpp IAppMonitorcli.cpp IDataProducercli.cpp IUserManagercli.cpp

libappmoncli_a_CPPFLAGS= $(libappmoncli_la_CPPFLAGS)
libappmoncli_a_HEADERS=AppMonitorcli.h SimpleAuthcli.h AppManagercli.h ../../appmon/monconst.h IUserManagercli.h IDataProducercli.h IAppMonitorcli.h IAppStorecli.h appmon.h appmonstruct.h 
libappmoncli_adir=$(includedir)/rpcf/appmoncli

libappmoncli_la_LIBADD= ./libappmoncli.a ../../registry/libregfs.la $(top_srcdir)/ipc/libipc.la $(top_srcdir)/combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L$(top_srcdir)/3rd_party
libappmoncli_la_LDFLAGS= -rdynamic -version-info @VERSION2@

libappmancli_la_LIBADD= ./libappmoncli.a ../../registry/libregfs.la $(top_srcdir)/ipc/libipc.la $(top_srcdir)/combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L$(top_srcdir)/3rd_party
libappmancli_la_LDFLAGS= -rdynamic -version-info @VERSION2@

libappmoncli_ladir=$(libdir)

headers=$(libappmoncli_a_HEADERS) 

sources=$(libappmoncli_a_SOURCES) AppMonitorcli.cpp maincli.cpp SimpleAuthcli.cpp AppManagercli.cpp 

$(sources):$(headers)

$(headers): appmon.ridl

appmon.ridl:
	bexit=0;for i in $(sources) $(headers);do if [ ! -f $${i} ];then bexit=1;break;fi;done; \
    if((bexit==0));then echo all files are ready;exit 0;fi; \
    if ! which ridlc > /dev/null; then \
        if [ ! -f ./mktpl ];then \
            cp ../../../ridl/{mktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f ../../../ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=../../../combase/.libs:$${LD_LIBRARY_PATH} ../../../ridl/.libs/ridlc --client -slO . ../../appmon/appmon.ridl; \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    ls

.PHONY: appmon.ridl
