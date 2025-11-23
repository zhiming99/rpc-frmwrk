appmonui_PROGRAMS=appmonui
appmonui_SOURCES=maincli.js appmon.html AppMonitorcli.js i18n-helper.js \
locales/zh.json locales/en.json appdetail.js README_cn.md Makefile.skel \
README.md webpack.config.js

EXEEXT=.tar.gz
files_to_package=appmon.html locales/zh.json locales/en.json i18n-helper.js \
appdetail.js appmondesc.json

ridlc_path=../../../../ridl/.libs/ridlc
ridl_file=../../../appmon/appmon.ridl

appmonuiconf_DATA=appmonui$(EXEEXT)
appmonuiconfdir=$(sysconfdir)/rpcf
appmonuidir=$(sysconfdir)/rpcf

auto_files=appmonstructs.js AppMonitorclibase.js driver.json appmondesc.json

appmon.ridl:
	@bexit=0;for i in $(auto_files);do if [ ! -f $${i} ];then bexit=1;break;fi;done; \
    if((bexit==0));then exit 0;fi; \
    if true; then \
        if [ ! -f ./mktpl ];then \
            cp ../../../../ridl/{mktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f ../../../../ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=../../../../combase/.libs:$${LD_LIBRARY_PATH} $(ridlc_path) --noreadme --services=AppMonitor -sJO . $(ridl_file); \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    python3 ../../../appmon/upddesc.py; \
	make -f Makefile.skel

$(auto_files): appmon.ridl

appmonui$(EXEEXT): $(auto_files) maincli.js AppMonitorcli.js
	tar zcf appmonui$(EXEEXT) $(files_to_package) dist/*

install-exec-hook:
	echo already installed

package:
	make -f Makefile.skel
	tar zcf appmonui.tar.gz $(files_to_package) dist/*

dbg-package:
	make -f Makefile.skel debug
	tar zcf appmonui.tar.gz $(files_to_package) dist/*
.PHONY: package dbg-package appmon.ridl 
