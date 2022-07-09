pysources = rpcfg.py updcfg.py rpcfgnui.py
EXTRA_DIST= $(pysources) README.md runtest.sh.in Dockerfile testcfgs/mincfg.json testcfgs/sslcfg.json.in testcfgs/server.key testcfgs/server.crt cfg4ssl.sh

docker:
	docker build -t rpcf .

install-exec-local:
	sed -e 's|[@]bindir[@]|${bindir}|g;s|[@]libdir[@]|${libdir}|g;s|[@]sysconfdir[@]|${sysconfdir}|g' runtest.sh.in > runtest.sh
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir);\
	else BinDir=$(DESTDIR)$(bindir); fi;\
	if [ ! -e $${BinDir}/rpcf ]; then mkdir -p $${BinDir}/rpcf;fi; \
	cp rpcfg.py updcfg.py rpcfgnui.py $${BinDir}/rpcf;\
    chmod +x $${BinDir}/rpcf/{rpcfg,updcfg,rpcfgnui}.py
