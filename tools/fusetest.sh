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
ulimit -n 8192
ulimit -a
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

umount mp
umount mpsvr

popd
popd

function pytest()
{
    testcase=$1
    pushd $testcase
    if [ -e ./maincli.py ]; then 
        cmdline=`awk '{if(FNR==2) print $0;}' maincli.py | sed -e 's/# //'`
    elif [ -e ./mainsvr.py ]; then
        cmdline=`awk '{if(FNR==2) print $0;}' mainsvr.py | sed -e 's/# //'`
    else
        echo "error cannot find file 'maincli.py' or 'mainsvr.py'"
        return 31
    fi

    ret=0
    echo ls /dev/fuse -l
    ls /dev/fuse -l
    echo 'ps aux | grep rpcrouter'
    ps aux | grep rpcrouter
    while true; do
        eval $cmdline
        echo make $testcase ...
        make || ret=32
        if (( $ret > 0 )); then break; fi
        echo create directories ...
        mkdir ./fs/mp ./fs/mpsvr > /dev/null 2>&1
        echo get filenames
        ridlfile=`echo $cmdline | awk '{print $NF}'`
        appname=`grep appname $ridlfile | awk '{print $NF}' | sed 's/[";]//g'`
        echo appname is $appname
        svcpt=`grep '^service' $ridlfile | awk '{print $2}'`
        echo svcpt is $svcpt
        pushd ./fs
        echo release/${appname}svr -f ./mpsvr
        release/${appname}svr -f ./mpsvr &
        sleep 2
        echo release/${appname}cli -f ./mp
        release/${appname}cli -f ./mp &
        popd
        ls -R .
        sleep 5
        python3 ./mainsvr.py fs/mpsvr/$svcpt 0 &
        sleep 3
        python3 ./maincli.py fs/mp/connection_0/$svcpt 0 || ret=37
        pid=`ps aux | grep 'mainsvr.py'| grep -v 'grep' | awk '{ print $2 }' `
        echo kill -9 $pid
        kill -9 $pid
        sleep 3
        break 1
    done
    umount ./fs/mp
    umount ./fs/mpsvr
    popd
    return $ret
}

pushd $pydir

pytest testypes2 || exit 1
pytest actcancel || exit 1
pytest iftest || exit 1
pytest hellowld || exit 1
pytest katest || exit 1
pytest evtest || exit 1

popd
