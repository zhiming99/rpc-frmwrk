#!/bin/bash
BASE=./
echo downloading GmSSL...
pushd ${BASE} ;
for((i=0;i<100;i++)); do
    if git clone 'https://github.com/zhiming99/GmSSL.git';then break; fi
done
popd

pushd ${BASE};cd ./GmSSL;mkdir build;cd build;cmake ..;make;make install; popd

echo downloading rpc-frmwrk...
pushd ${BASE};
for((i=0;i<100;i++)); do
    if git clone 'https://github.com/zhiming99/rpc-frmwrk.git'; then break; fi
done
popd
pushd ${BASE}/rpc-frmwrk; libtoolize && aclocal && autoreconf -vfi && \
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`; popd
pushd ${BASE}/rpc-frmwrk && bash cfgsel -r && make -j 4; popd;
pushd ${BASE}/rpc-frmwrk; python3 tools/rpcfg.py; sudo make install; popd
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
echo Congratulations! build complete.
