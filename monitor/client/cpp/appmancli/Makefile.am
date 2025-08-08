link_dir=$(top_srcdir)/combase/
include_dir=$(top_srcdir)/ipc
mon_dir=$(top_srcdir)/monitor
top_dir=$(top_srcdir)

lib_LTLIBRARIES=libappmancli.la
libappmancli_la_SOURCES= maincli.cpp AppManagercli.cpp SimpleAuthcli.cpp appmon.cpp appmonstruct.cpp IAppStorecli.cpp IDataProducercli.cpp IUserManagercli.cpp AppManagercli.h SimpleAuthcli.h
libappmancli_la_CPPFLAGS= -I. -I$(mon_dir)/appmon -I$(mon_dir)/registry -I$(top_srcdir)/include -I$(include_dir) -fPIC $(OFFSET_MACRO) 

EXTRA_DIST=crontest.cpp

appmancliincdir=$(includedir)/rpcf/appmancli
appmancliinc_HEADERS=SimpleAuthcli.h AppManagercli.h IUserManagercli.h IDataProducercli.h IAppStorecli.h

libappmancli_la_LIBADD= $(mon_dir)/registry/libregfs.la $(top_srcdir)/ipc/libipc.la $(top_srcdir)/combase/libcombase.la -lpthread -ldl $(COMMON_LINK) -L$(top_srcdir)/3rd_party
libappmancli_la_LDFLAGS= -rdynamic -version-info @VERSION2@

headers=SimpleAuthcli.h AppManagercli.h $(mon_dir)/appmon/monconst.h IUserManagercli.h IDataProducercli.h appmon.h appmonstruct.h IAppStorecli.h
sources=$(libappmancli_la_SOURCES)

$(sources):$(headers)

$(headers):appmon.ridl

appmon.ridl:
	@bexit=0;for i in $(sources) $(headers);do if [ ! -f $${i} ];then bexit=1;break;fi;done; \
    if((bexit==0));then echo all files are ready;exit 0;fi; \
    if true; then \
        if [ ! -f ./mktpl ];then \
            cp $(top_dir)/ridl/{mktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f $(top_dir)/ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=$(top_dir)/combase/.libs:$${LD_LIBRARY_PATH} \
        $(top_dir)/ridl/.libs/ridlc --sync_mode IAppStore=async_p --sync_mode IUserManager=async_p \
        --services=AppManager,SimpleAuth --client -slO . $(mon_dir)/appmon/appmon.ridl; \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    ls -x

apptimer.cpp: $(lib_LTLIBRARIES)

apptimer_PROGRAMS=apptimer
apptimer_SOURCES=apptimer.cpp cronexpr.cpp cronexpr.h
apptimer_LDADD=libappmancli.la $(top_srcdir)/ipc/libipc.la \
$(top_srcdir)/combase/libcombase.la -lutils -lstdc++ -lpthread -ldl $(COMMON_LINK) -L$(top_srcdir)/3rd_party -L$(abs_top_srcdir)/combase -L$(top_srcdir)/ipc
apptimer_CPPFLAGS= -I. -I$(mon_dir)/appmon -I$(mon_dir)/registry -I$(top_srcdir)/include -I$(include_dir) -fPIC $(OFFSET_MACRO)
apptimer_LDFLAGS=-rdynamic -version-info @VERSION2@
apptimerdir=$(bindir)/rpcf

.PHONY: appmon.ridl
