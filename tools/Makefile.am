pysources = rpcfg.py
EXTRA_DIST= $(pysources) README.md Dockerfile
install-exec-local:
	-if [ ! -e $(bindir)/rpcf ]; then mkdir -p $(bindir)/rpcf;fi
	-cp rpcfg.py $(bindir)/rpcf

