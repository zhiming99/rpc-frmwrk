link_dir=$(top_srcdir)/combase/
include_dir=$(top_srcdir)/ipc
mon_dir=$(top_srcdir)/monitor
top_dir=$(top_srcdir)

lib_LTLIBRARIES=libappmoncli.la
libappmoncli_la_SOURCES= maincli.cpp AppMonitorcli.cpp appmon.cpp appmonstruct.cpp IAppStorecli.cpp IAppMonitorcli.cpp AppMonitorcli.h 
libappmoncli_la_CPPFLAGS= -I. -I$(mon_dir)/registry -I$(mon_dir)/appmon -I$(top_srcdir)/include -I$(include_dir) -fPIC $(OFFSET_MACRO)

EXTRA_DIST=

libappmoncli_la_HEADERS=
libappmoncli_ladir=$(includedir)/rpcf/moncli

libappmoncli_la_LIBADD= $(mon_dir)/registry/libregfs.la $(top_srcdir)/ipc/libipc.la $(top_srcdir)/combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L$(top_srcdir)/3rd_party
libappmoncli_la_LDFLAGS= -rdynamic -version-info @VERSION2@

headers=$(libappmoncli_la_HEADERS) appmon.h appmonstruct.h IAppMonitorcli.h IAppStorecli.h
sources=$(libappmoncli_la_SOURCES) AppMonitorcli.cpp maincli.cpp

$(sources):$(headers)

$(headers):appmon.ridl

appmon.ridl:
	bexit=0;for i in $(sources) $(headers);do if [ ! -f $${i} ];then bexit=1;break;fi;done; \
    if((bexit==0));then echo all files are ready;exit 0;fi; \
    if ! which ridlc > /dev/null; then \
        if [ ! -f ./mktpl ];then \
            cp $(top_dir)/ridl/{mktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f $(top_dir)/ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=$(top_dir)/combase/.libs:$${LD_LIBRARY_PATH} $(top_dir)/ridl/.libs/ridlc --services=AppMonitor --client -slO . $(mon_dir)/appmon/appmon.ridl; \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    ls -x

.PHONY: appmon.ridl
