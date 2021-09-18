pysources = rpcfg.py updcfg.py rpcfgnui.py
EXTRA_DIST= $(pysources) README.md runtest.sh Dockerfile testcfgs/mincfg.json

docker:
	docker build -t rpcf .

install-exec-local:
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir);\
	else BinDir=$(DESTDIR)$(bindir); fi;\
	if [ ! -e $${BinDir}/rpcf ]; then mkdir -p $${BinDir}/rpcf;fi; \
	cp rpcfg.py updcfg.py rpcfgnui.py $${BinDir}/rpcf;
