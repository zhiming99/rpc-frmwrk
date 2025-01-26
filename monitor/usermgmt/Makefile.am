man_MANS= rpcfaddu.1 rpcfmodu.1 rpcfshow.1 updattr.1

bindist=inituser.sh pubfuncs.sh rpcfaddg.sh rpcfaddu.sh \
rpcfmodu.sh rpcfrmg.sh rpcfrmu.sh rpcfshow.sh updattr.py 

EXTRA_DIST=$(bindist) $(man_MANS)

if ENABLE_FUSE3
scripts_DATA=$(bindist)
scriptsdir=$(bindir)/rpcf
endif
