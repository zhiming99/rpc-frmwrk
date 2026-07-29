#!/bin/bash
BASE=./
echo downloading GmSSL...
pushd ${BASE} ;
if [ -f ./GmSSL/.git/HEAD ]; then
    pushd GmSSL
    for((i=0;i<100;i++)); do
        if git pull origin master;then break; fi
    done
    popd
else
    for((i=0;i<100;i++)); do
        if git clone 'https://github.com/zhiming99/GmSSL.git';then break; fi
    done
fi
popd

pushd ${BASE};cd ./GmSSL;mkdir build;cd build;cmake ..;make;make install; popd

echo downloading rpc-frmwrk...
pushd ${BASE};
if [ -f ./rpc-frmwrk/.git/HEAD ]; then
    pushd rpc-frmwrk
    for((i=0;i<100;i++)); do
        if git pull origin master;then break; fi
    done
    popd
else 
    for((i=0;i<100;i++)); do
        if git clone 'https://github.com/zhiming99/rpc-frmwrk.git'; then break; fi
    done
fi
popd

if [ -z "$1" ]; then
    pushd ${BASE}/rpc-frmwrk; libtoolize && aclocal && autoreconf -vfi && \
    automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`; popd
    pushd ${BASE}/rpc-frmwrk && bash cfgsel -r && make -j 4; popd;
    pushd ${BASE}/rpc-frmwrk; ${SUDO} make install; popd
elif [ "$1" == "cmake" ];then
    pushd ${BASE}/rpc-frmwrk
    mkdir build
    pushd build
    cmake ..
    cmake --build .
    ${SUDO} cmake --install .
    popd
    popd
fi
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
echo Congratulations! build complete. 
echo Please make sure to run 'rpcfctl cfg' or 'rpcfctl tui' to config the system.
