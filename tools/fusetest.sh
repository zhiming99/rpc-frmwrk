#!/bin/bash
basedir=`pwd`
pushd examples/cpp/
if [ -d testypes ]; then
    rm -rf testypes/* || true
else
    mkdir testypes
fi

ulimit -n 8192
ulimit -a

function stressTest()
{
    pushd testypes
    make || exit 10

    mkdir mp mpsvr || true
    release/TestTypessvr -f mpsvr &
    release/TestTypescli -f mp &
    sleep 5 

    pydir=$basedir/fuse/examples/python
    python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

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

    popd
    rm -rf testypes/*
}


function mkDirTest()
{
    pushd testypes
    make || exit 10

    mkdir mp mpsvr || true
    hostsvr -f mpsvr &
    hostcli -f mp &
    sleep 5 
    echo loading TestTypes library to server and proxy...
    echo "loadl $(pwd)/release/libTestTypes.so" > ./mpsvr/commands
    echo "loadl $(pwd)/release/libTestTypes.so" > ./mp/commands
    echo adding service point TestTypes to server and proxy...
    echo "addsp TestTypesSvc $(pwd)/TestTypesdesc.json" > ./mpsvr/commands
    echo "addsp TestTypesSvc $(pwd)/TestTypesdesc.json" > ./mp/commands

    pydir=$basedir/fuse/examples/python
    python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

/bin/bash << RUNCLIENT
function singleMkdir()
{
    idx=0
    targdir=TestTypesSvc_\$1
    conndir=connection_\$idx
    pushd ./mp
    if [ -d ./\$conndir/\$targdir ]; then
        rmdir ./\$conndir/\$targdir
    fi
    echo mkdir \$targdir
    mkdir \$targdir
    popd
    python3 $pydir/testypes/maincli.py ./\$conndir/\$targdir 0
    pushd ./mp/
    rmdir ./\$conndir/\$targdir
    popd
}
start=\$(date +%s.%N)
for((i=0;i<200;i++));do
    singleMkdir \$i &
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

    popd
    rm -rf testypes/*
}

echo testing normal RPC
$bin_dir/ridlc -f -O ./testypes ../testypes.ridl
stressTest

echo testing RPC-over-stream
$bin_dir/ridlc -sf -O ./testypes ../testypes.ridl
stressTest

echo testing normal RPC
$bin_dir/ridlc -lf -O ./testypes ../testypes.ridl
mkDirTest

echo testing RPC-over-stream
$bin_dir/ridlc -lsf -O ./testypes ../testypes.ridl
mkDirTest

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

