#!/bin/bash
basedir=`pwd`
pushd examples/cpp/
if [ -d testypes ]; then
    make -C testypes clean
    rm testypes/* || true
else
    mkdir testypes
fi

$bindir/ridlc -f -O ./testypes ../testypes.ridl
pushd testypes
make || exit 10

mkdir mp mpsvr || true
release/TestTypessvr mpsvr &
release/TestTypescli mp &

pydir=$basedir/fuse/examples/python
if [ ! -e $pydir/testypes/iolib.py ]; then
    ln -s $pydir/iolib.py testypes/ || exit 10
fi
sleep 5 

python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

start=$(date +%s.%N)
for((i=0;i<100;i++));do
    python3 $pydir/testypes/maincli.py mp/connection_0/TestTypesSvc $i &
done

#wait
for((i=0;i<1000;i++)); do
count=`jobs | grep 'maincli.py'|wc -l`
if [ count == '100' ]; then
    break
fi
done

end=$(date +%s.%N)
echo "scale=10;$end-$start" | bc

kill -9 `ps aux | grep mainsvr | grep -v grep | awk '{print $2}'`

fusermount -u mp
fusermount -u mpsvr

popd
popd

