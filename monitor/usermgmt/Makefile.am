man_MANS= rpcfaddu.1 rpcfmodu.1 rpcfshow.1 updattr.1

bindist=inituser.sh pubfuncs.sh rpcfaddg.sh rpcfaddu.sh \
rpcfmodu.sh rpcfrmg.sh rpcfrmu.sh rpcfshow.sh updattr.py 

EXTRA_DIST= $(man_MANS) README.md README_cn.md

if ENABLE_FUSE3
dist_usrmgt_SCRIPTS=$(bindist)
usrmgtdir=$(bindir)/rpcf
endif

