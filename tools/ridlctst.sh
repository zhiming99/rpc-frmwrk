#!/bin/sh
lib_dir=/usr/local/lib
export PATH=/usr/lib/ccache:/usr/local/bin:/usr/local/bin/rpcf:$PATH
export LD_LIBRARY_PATH=${lib_dir}:${lib_dir}/rpcf
mkdir testridlc
pushd testridlc
a=(../ridl/fulltest.ridl ../monitor/appmon/appmon.ridl)

for i in ${a[@]}; do

rm -rf *.cpp *.h *.new
echo ridlc -lsf -O . $i
ridlc -lsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -bsf -O . $i
ridlc --server -bsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -lsf -O . $i
ridlc --client -lsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -sf -O . $i
ridlc --client -sf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
echo rm -rf *.cpp *.h *.new
rm -rf *.cpp *.h *.new
echo ridlc --client -s -O . $i
ridlc --client -s -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -f -O . $i
ridlc --client -f -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -lsf -O . $i
ridlc --server -lsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -sf -O . $i
ridlc --server -sf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -s -O . $i
ridlc --server -s -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -f -O . $i
ridlc --server -f -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc -bsf -O . $i
ridlc -bsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -bsf -O . $i
ridlc --client -bsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -bf -O . $i
ridlc --client -bf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -bs -O . $i
ridlc --client -bs -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --client -b -O . $i
ridlc --client -b -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -bsf -O . $i
ridlc --server -bsf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -bf -O . $i
ridlc --server -bf -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -bs -O . $i
ridlc --server -bs -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi

make clean
rm -rf *.cpp *.h *.new
echo ridlc --server -b -O . $i
ridlc --server -b -O . $i
if ! make debug -j4 > templog 2>&1; then
    cat templog
    exit 1
fi
done

make clean
rm -rf *.cpp *.h *.new
echo testing sync_mode 1
echo ridlc --server --sync_mode IUserManager=async --sync_mode IAppStore.ListApps=sync --services AppManager,SimpleAuth -l -O . ../monitor/appmon/appmon.ridl
ridlc --server --sync_mode IUserManager=async --sync_mode IAppStore.ListApps=sync --services AppManager,SimpleAuth -l -O . ${a[1]}
make debug -j4 >templog 2>&1 || ( cat templog; exit 1 )

make clean
rm -rf *.cpp *.h *.new
echo testing sync_mode 2
echo ridlc --client --sync_mode IUserManager=async --sync_mode IAppStore=async_p --services AppManager,SimpleAuth -l -O . ${a[1]}
ridlc --client --sync_mode IUserManager=async --sync_mode IAppStore=async_p --services AppManager,SimpleAuth -l -O . ${a[1]}
make debug -j4 >templog 2>&1 || ( cat templog; exit 1 )
popd
