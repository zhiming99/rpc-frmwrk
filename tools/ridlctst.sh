#!/bin/sh
lib_dir=/usr/local/lib
export PATH=/usr/lib/ccache:/usr/local/bin:/usr/local/bin/rpcf:$PATH
export LD_LIBRARY_PATH=${lib_dir}:${lib_dir}/rpcf
mkdir testridlc
pushd testridlc
a=(../ridl/fulltest.ridl ../monitor/appmon/appmon.ridl)

for i in ${a[@]}; do
rm -rf *.cpp *.h *.new
ridlc -lsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -bsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -lsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -sf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -s -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -f -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -lsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -sf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -s -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -f -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc -bsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -bsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -bf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -bs -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --client -b -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -bsf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -bf -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -bs -O . $i
if ! make debug -j4; then
    exit 1
fi

rm -rf *.cpp *.h *.new
ridlc --server -b -O . $i
if ! make debug -j4; then
    exit 1
fi
done

rm -rf *.cpp *.h *.new
ridlc --server --sync_mode IUserManager=async --sync_mode \
IAppStore.ListApps=sync --services AppManager,SimpleAuth -l -O . \
${a[1]} || exit 1

rm -rf *.cpp *.h *.new
ridlc --client --sync_mode IUserManager=async --sync_mode \
IAppStore=async_p --services AppManager,SimpleAuth -l -O . \
${a[1]} || exit 1

popd
