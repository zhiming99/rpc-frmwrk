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
release/TestTypessvr -f mpsvr &
release/TestTypescli -f mp &

pydir=$basedir/fuse/examples/python
sleep 5 

python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &
ulimit -n 8192
ulimit -a
/bin/bash << RUNCLIENT
start=\$(date +%s.%N)
for((i=0;i<200;i++));do
    python3 $pydir/testypes/maincli.py mp/connection_0/TestTypesSvc \$i &
done
wait \`jobs -p\`
end=\$(date +%s.%N)
echo -n "time elapsed: "
echo "scale=10;\$end-\$start" | bc
RUNCLIENT


echo kill -9 `ps aux | grep mainsvr | grep -v grep | awk '{print $2}'`
kill -9 `ps aux | grep mainsvr | grep -v grep | awk '{print $2}'`

umount mp
while true; do
    umount mpsvr 
    if mount | grep TestTypessvr; then
        sleep 1
        continue
    fi
    break
done

rm -rf ./testypes

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
        echo release/${appname}svr ./mpsvr
        release/${appname}svr ./mpsvr
        sleep 2
        echo release/${appname}cli -f ./mp
        release/${appname}cli ./mp
        popd
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

    while true; do
        umount ./fs/mpsvr 
        if mount | grep ${appname}svr; then
            sleep 1
            continue
        fi
        break
    done

    rm *.new
    rm -rf fs
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

