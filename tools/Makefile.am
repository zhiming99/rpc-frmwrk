pysources = rpcfg.py updcfg.py rpcfgnui.py
EXTRA_DIST= $(pysources) README.md Dockerfile runtest.sh.in
install-exec-local:
	-if [ ! -e $(bindir)/rpcf ]; then mkdir -p $(bindir)/rpcf;fi
	cp rpcfg.py updcfg.py rpcfgnui.py $(bindir)/rpcf
	sed -e 's,[@]bindir[@],$(bindir),g;s,[@]libdir[@],$(libdir),g' runtest.sh.in > runtest.sh

