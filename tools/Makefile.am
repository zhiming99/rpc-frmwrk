pysources = rpcfg.py rpcfgnui.py updinitcfg.py updwscfg.py krbparse.py \
updk5cfg.py disablessl.py geninitcfg.py

testcfgfiles=testcfgs/mincfg.json \
testcfgs/sslcfg.json.in testcfgs/server.key testcfgs/server.crt cfg4ssl.sh

#gmssl test files
testcfgfiles +=  \
cfg4gmssl.sh updgmskey.py testcfgs/gmsslcfgs/signkey.pem \
testcfgs/gmsslcfgs/certs.pem testcfgs/gmsslcfgs/signcert.pem \
testcfgs/gmsslcfgs/clientkey.pem testcfgs/gmsslcfgs/clientcert.pem \
testcfgs/gmsslcfgs/initcfg.json.in

runtime_files= 

if ENABLE_GMSSL
#gmssl files
runtime_files += gmsslkey.sh
gmssl_cmd=install gmsslkey.sh $${BinDir}/rpcf/;
else
gmssl_cmd=true;
endif

if ENABLE_OPENSSL
#openssl files
    runtime_files +=  opensslkey.sh testcfgs/openssl.cnf
    openssl_cmd=install opensslkey.sh $${BinDir}/rpcf/

    #installation information
    dist_testcfg_DATA=testcfgs/openssl.cnf
    testcfgdir=$(sysconfdir)/rpcf
else
    openssl_cmd=true;
endif

EXTRA_DIST= $(pysources) README.md README_cn.md runtest.sh.in ridlctst.sh updcfg.py.in Dockerfile buildall-deb.sh buildall-fed.sh makerpcf.sh buildall-rasp.sh makerpcf-rasp.sh btinrtst.sh $(testcfgfiles) $(runtime_files)

docker:
	docker build -t rpcf .

install-exec-local:
	sed -e 's|[@]bindir[@]|${bindir}|g;s|[@]libdir[@]|${libdir}|g;s|[@]sysconfdir[@]|${sysconfdir}|g' runtest.sh.in > runtest.sh
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir);\
	else BinDir=$(DESTDIR)$(bindir); fi;\
	if [ ! -e $${BinDir}/rpcf ]; then mkdir -p $${BinDir}/rpcf;fi; \
	install rpcfg.py updcfg.py rpcfgnui.py updinitcfg.py updwscfg.py krbparse.py updk5cfg.py disablessl.py geninitcfg.py $${BinDir}/rpcf/;\
    chmod +x $${BinDir}/rpcf/{rpcfg,updcfg,rpcfgnui}.py;\
    eval ${gmssl_cmd}\
    eval ${openssl_cmd}
    
uninstall-hook:
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir); else BinDir=$(DESTDIR)$(bindir); fi;\
    pushd $${BinDir}/rpcf; rm ${pysources}; popd
