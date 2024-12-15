#!/bin/bash
BASE=./
echo downloading rpc-frmwrk...
if [ ! -f ../ipc/rpcif.cpp ]; then
	pushd ${BASE};
	for((i=0;i<100;i++)); do
	    if git clone 'https://github.com/zhiming99/rpc-frmwrk.git'; then break; fi
	done
	popd
else
	BASE=../../
fi
pushd ${BASE}/rpc-frmwrk; libtoolize && aclocal && autoreconf -vfi && \
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`; popd
pushd ${BASE}/rpc-frmwrk
bash ./cfgsel -r --disable-js --disable-java --disable-gmssl
make
popd

pushd ${BASE}/rpc-frmwrk; ${SUDO} make install; popd
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
echo Congratulations! build complete. Please remember to run rpcfg.py to config the system.
