link_dir=$(top_srcdir)/combase/
include_ipcdir=$(top_srcdir)/ipc

appmon_PROGRAMS=appmonsvr
appmonsvr_SOURCES=mainsvr.cpp AppMonitorsvr.cpp AppMonitorsvr.h monconst.h \
SimpleAuthsvr.cpp SimpleAuthsvr.h AppManagersvr.cpp AppManagersvr.h
if ENABLE_FUSE3
appmonsvr_SOURCES+=monfuse.cpp
endif
#appmoncli_SOURCES=maincli.cpp AppMonitorcli.cpp  monconst.h\
#SimpleAuthcli.cpp AppManagercli.cpp 

appmonsvr_LDADD= ./libappmon.a \
../registry/libregfs.la ../../ipc/libipc.la $(COMMON_LINK_FUSE3) \
 ../../combase/libcombase.la -lutils -lstdc++ -lcppunit \
 -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc \
 -L$(top_srcdir)/3rd_party 

if ENABLE_OPENSSL
appmonsvr_LDADD+= $(COMMON_LINK_SSL)
endif

if ENABLE_GMSSL
appmonsvr_LDADD+= -L$(libdir) -lgmssl 
endif

#appmoncli_LDADD=$(appmonsvr_LDADD)

appmonconf_DATA=appmondesc.json appmon.ridl
appmonconfdir=$(sysconfdir)/rpcf
appmondir=$(bindir)

appmonincdir=$(includedir)/rpcf/appmancli
appmoninc_HEADERS=appmon.h monconst.h appmonstruct.h

if BITWIDTH32
OFFSET_MACRO=-D_FILE_OFFSET_BITS=64
else
OFFSET_MACRO=
endif

appmonsvr_CPPFLAGS= -I../registry -I$(include_ipcdir) -fPIC $(OFFSET_MACRO) 
appmonsvr_LDFLAGS=-rdynamic

if ENABLE_FUSE3
appmonsvr_CPPFLAGS+= -DFUSE3
dist_appmonscr_SCRIPTS=initappreg.sh appfuncs.sh \
addapp.sh addpoint.sh chmodapp.sh getpv.sh listapps.sh rmapp.sh rmlink.sh setpv.sh showpoint.sh addlink.sh chownapp.sh rmattr.sh rmpoint.sh setattr.sh showapp.sh

appmonscrdir=$(bindir)/rpcf
endif

#appmoncli_CPPFLAGS=$(appmonsvr_CPPFLAGS)
#appmoncli_LDFLAGS=$(appmonsvr_LDFLAGS)

EXTRA_DIST=appmon.ridl upddesc.py

noinst_LIBRARIES=libappmon.a
libappmon_a_SOURCES = appmon.cpp appmonstruct.cpp IAppStoresvr.cpp IAppMonitorsvr.cpp IDataProducersvr.cpp IUserManagersvr.cpp appmondesc.json IUserManagersvr.h IDataProducersvr.h IAppMonitorsvr.h IAppStoresvr.h appmon.h appmonstruct.h 
libappmon_a_CPPFLAGS= $(appmonsvr_CPPFLAGS)
libappmon_a_HEADERS=

libappmon_adir=$(includedir)/rpcf/appmon
#libappmon_la_LIBADD= ../registry/libregfs.la ../../ipc/libipc.la ../../combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L../../3rd_party
#libappmon_la_LDFLAGS= -version-info @VERSION2@
#libappmon_ladir=$(libdir)

headers=$(libappmon_a_HEADERS) AppMonitorsvr.h SimpleAuthsvr.h AppManagersvr.h monconst.h

sources=$(libappmon_a_SOURCES) AppMonitorsvr.cpp mainsvr.cpp SimpleAuthsvr.cpp AppManagersvr.cpp 

$(sources):$(headers)

$(headers): appmon.ridl ../registry/regfsif.ridl

appmon.ridl:
	@bexit=0;for i in $(sources) $(headers);do if [ ! -f $${i} ];then bexit=1;break;fi;done; \
    if((bexit==0));then exit 0;fi; \
    if ! which ridlc > /dev/null; then \
        if [ ! -f ./mktpl ];then \
            cp ../../ridl/{mktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f ../../ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=../../combase/.libs:$${LD_LIBRARY_PATH} ../../ridl/.libs/ridlc --server -sO . ./appmon.ridl; \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    sed -i 's/EnumMyClsid/EnumAMClsid/' appmon.h ;\
    python3 upddesc.py

.PHONY: appmon.ridl
