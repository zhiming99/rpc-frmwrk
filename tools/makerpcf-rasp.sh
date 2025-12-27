#!/bin/bash
#local script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
echo downloading GmSSL...

if [ ! -d ./GmSSL -o ! -f ./GmSSL/CMakeLists.txt ]; then
	for((i=0;i<100;i++)); do
	    if git clone 'https://github.com/zhiming99/GmSSL.git'; then break; fi
	done
    if ((i==100));then
        echo Error failed to pull GmSSL
        exit 1
    fi
	popd
fi
pushd ./GmSSL
if [ ! -d build ]; then mkdir ./build; fi
	cd build
	cmake ..
	make;make test
	${SUDO} make install
popd

echo downloading rpc-frmwrk...
if [ ! -d rpc-frmwrk -o ! -f ./rpc-frmwrk/ipc/rpcif.cpp ]; then
	for((i=0;i<100;i++)); do
	    if git clone 'https://github.com/zhiming99/rpc-frmwrk.git'; then break; fi
	done
    if ((i==100)); then
        echo Error failed to pull rpc-frmwrk
        exit 1
    fi
	popd
fi

pushd ./rpc-frmwrk; autoreconf -vfi &&
automake --add-missing && autoconf; echo `pwd`;ls -l `pwd`;

bash ./cfgsel -r
make

${SUDO} make install;
popd
echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/rpcf'>>${HOME}/.bashrc
echo Congratulations! build complete. Please remember to run rpcfg.py to config the system.
