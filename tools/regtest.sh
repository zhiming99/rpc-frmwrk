#!/bin/bash
if ! wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.10.tar.xz > /dev/null; then
    exit 1
fi

lib_dir=/usr/local/lib
export LD_LIBRARY_PATH=${lib_dir}:${lib_dir}/rpcf

mkdir ./regtest
mntdir=./regtest
/usr/local/bin/regfsmnt -i registry.dat
/usr/local/bin/regfsmnt -sd registry.dat $mntdir
cnt=0
while ! mountpoint -q $mntdir; do
    sleep 1
    ((cnt++))
    if [ $cnt -gt 10 ]; then
        echo "Failed to mount registry.dat at $mntdir"
        exit 1
    fi
done
echo creating files and directories test...
tar Jxf linux-6.12.10.tar.xz -C $mntdir
umount $mntdir
echo checking registry
/usr/local/bin/regfschk registry.dat > regout 2>&1
grep -i Error regout 

echo deleting files and directories test...
/usr/local/bin/regfsmnt -sd registry.dat $mntdir
cnt=0
while ! mountpoint -q $mntdir; do
    sleep 1
    ((cnt++))
    if [ $cnt -gt 10 ]; then
        echo "Failed to mount registry.dat at $mntdir"
        exit 1
    fi
done
tar Jxf linux-6.12.10.tar.xz -C $mntdir
ls -l $mntdir/
umount $mntdir
echo checking registry again...
/usr/local/bin/regfschk registry.dat > regout 2>&1
rm registry.dat linux-6.12.10.tar.xz
if grep -i Error regout > /dev/null; then
    echo Error deleting files...
    exit 1
fi
exit 0
