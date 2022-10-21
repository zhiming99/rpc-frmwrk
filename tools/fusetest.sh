#!/bin/bash
basedir=`pwd`
pushd examples/cpp/
if [ -d testypes ]; then
    make -C testypes clean
    rm testypes/* || true
else
    mkdir testypes
fi

$bin_dir/ridlc -f -O ./testypes ../testypes.ridl
pushd testypes
make || exit 10

mkdir mp mpsvr || true
release/TestTypessvr mpsvr &
release/TestTypescli mp &

pydir=$basedir/fuse/examples/python
sleep 5 

python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

start=$(date +%s.%N)
for((i=0;i<200;i++));do
    python3 $pydir/testypes/maincli.py mp/connection_0/TestTypesSvc $i &
done

#wait
for((i=0;i<1000;i++)); do
count=`jobs | grep 'maincli.py'|wc -l`
if [ count == '200' ]; then
    break
fi
done

end=$(date +%s.%N)
echo "scale=10;$end-$start" | bc

echo kill -9 `ps aux | grep mainsvr | grep -v grep | awk '{print $2}'`
kill -9 `ps aux | grep mainsvr | grep -v grep | awk '{print $2}'`

fusermount3 -u mp
fusermount3 -u mpsvr

popd
popd

