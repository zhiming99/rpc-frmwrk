pysources = rpcfg.py updcfg.py rpcfgnui.py

testcfgfiles=testcfgs/mincfg.json \
testcfgs/sslcfg.json.in testcfgs/server.key testcfgs/server.crt cfg4ssl.sh \
gmsslkey.sh cfg4gmssl.sh updgmskey.py testcfgs/gmsslcfgs/signkey.pem \
testcfgs/gmsslcfgs/certs.pem testcfgs/gmsslcfgs/cacert.pem \
testcfgs/gmsslcfgs/clientkey.pem testcfgs/gmsslcfgs/clientcert.pem \
testcfgs/gmsslcfgs/rootcacert.pem testcfgs/gmsslcfgs/initcfg.json.in

EXTRA_DIST= $(pysources) README.md runtest.sh.in Dockerfile $(testcfgfiles)

docker:
	docker build -t rpcf .

install-exec-local:
	sed -e 's|[@]bindir[@]|${bindir}|g;s|[@]libdir[@]|${libdir}|g;s|[@]sysconfdir[@]|${sysconfdir}|g' runtest.sh.in > runtest.sh
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir);\
	else BinDir=$(DESTDIR)$(bindir); fi;\
	if [ ! -e $${BinDir}/rpcf ]; then mkdir -p $${BinDir}/rpcf;fi; \
	cp rpcfg.py updcfg.py rpcfgnui.py gmsslkey.sh $${BinDir}/rpcf;\
    chmod +x $${BinDir}/rpcf/{rpcfg,updcfg,rpcfgnui}.py
