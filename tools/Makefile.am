pysources = rpcfg.py updcfg.py rpcfgnui.py
EXTRA_DIST= $(pysources) README.md Dockerfile runtest.sh.in
install-exec-local:
	if [ "x$(DESTDIR)" == "x" ];then BinDir=$(bindir);\
	else BinDir=$(DESTDIR)$(bindir); fi;\
	if [ ! -e $${BinDir}/rpcf ]; then mkdir -p $${BinDir}/rpcf;fi; \
	cp rpcfg.py updcfg.py rpcfgnui.py $${BinDir}/rpcf; \
	ls -l runtest.sh.in; \
	sed -e 's,[@]bindir[@],$(bindir),g;s,[@]libdir[@],$(libdir),g' runtest.sh.in > runtest.sh;\
	if [ "x$${SUDO_USER}" != "x" ]; then chown $${SUDO_USER} runtest.sh || true;fi
