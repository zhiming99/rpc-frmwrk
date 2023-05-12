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
    cat ./cmdline
    make > /dev/null 2>&1 || exit 10

    mkdir mp mpsvr || true
    release/TestTypessvr -m mpsvr &
    sleep 8 
    release/TestTypescli -m mp &
    sleep 4

    #make sure TestTypesSvc created
    pushd mp
    echo mkdir TestTypesSvc_1
    mkdir TestTypesSvc_1 >/dev/null 2>&1
    popd

    echo ls mp
    ls -lR ./mp

    echo ls mpsvr
    ls -lR ./mpsvr

    pydir=$basedir/fuse/examples/python
    python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

/bin/bash << RUNCLIENT |& tee -a $basedir/logdump.txt
start=\$(date +%s.%N)
for((i=0;i<200;i++));do
    python3 $pydir/testypes/maincli.py mp/connection_0/TestTypesSvc \$i &
done
wait \`jobs -p\`
end=\$(date +%s.%N)
echo -n "time elapsed: "
echo "scale=10;\$end-\$start" | bc
RUNCLIENT

    echo pkill -f mainsvr.py
    pkill -f mainsvr.py

    echo umount mp
    umount mp

    echo umount mpsvr...
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
    cat ./cmdline
    make > /dev/null 2>&1 || exit 10

    mkdir mp mpsvr || true
    hostsvr -m mpsvr &
    hostcli -m mp &
    sleep 5 

    echo loading TestTypes library to server and proxy...
    echo "loadl $(pwd)/release/libTestTypes.so" > ./mpsvr/commands
    echo "loadl $(pwd)/release/libTestTypes.so" > ./mp/commands
    echo adding service point TestTypes to server and proxy...
    echo "addsp TestTypesSvc $(pwd)/TestTypesdesc.json" > ./mpsvr/commands
    echo "addsp TestTypesSvc $(pwd)/TestTypesdesc.json" > ./mp/commands
    sleep 3

    echo ls mp
    ls -lR ./mp

    echo ls mpsvr
    ls -lR ./mpsvr

    pydir=$basedir/fuse/examples/python
    python3 $pydir/testypes/mainsvr.py mpsvr/TestTypesSvc 0 &

/bin/bash << RUNCLIENT |& tee -a $basedir/logdump.txt
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
    mkdir \$targdir 2>/dev/null
    python3 $pydir/testypes/maincli.py ./\$conndir/\$targdir 0
    rmdir ./\$conndir/\$targdir
    popd
}
start=\$(date +%s.%N)
for((i=0;i<199;i++));do
    singleMkdir \$i &
done
wait \`jobs -p\`
end=\$(date +%s.%N)
echo -n "time elapsed: "
echo "scale=10;\$end-\$start" | bc
RUNCLIENT

    echo pkill -f mainsvr.py
    pkill -f mainsvr.py

    echo umount mp
    while true; do
        umount ./mp 
        if mount | grep 'TestTypescli\|hostcli'; then
            sleep 1
            continue
        fi
        break
    done

    echo umount mpsvr...
    while true; do
        umount mpsvr 
        if mount | grep 'TestTypessvr\|hostsvr'; then
            sleep 1
            continue
        fi
        break
    done

    popd
    rm -rf testypes/*
}

echo testing normal RPC
echo $bin_dir/ridlc -f -O ./testypes ../testypes.ridl
$bin_dir/ridlc -f -O ./testypes ../testypes.ridl
echo stressTest normal
stressTest

echo testing RPC-over-stream
echo $bin_dir/ridlc -sf -O ./testypes ../testypes.ridl
$bin_dir/ridlc -sf -O ./testypes ../testypes.ridl
echo stressTest ROS
stressTest

echo testing normal RPC via shared library
echo $bin_dir/ridlc -lf -O ./testypes ../testypes.ridl
$bin_dir/ridlc -lf -O ./testypes ../testypes.ridl
echo mkDirTest normal
mkDirTest

echo testing RPC-over-stream via shared library
echo $bin_dir/ridlc -lsf -O ./testypes ../testypes.ridl
$bin_dir/ridlc -lsf -O ./testypes ../testypes.ridl
echo mkDirTest ROS
mkDirTest

echo Check errors in logdump.txt...
if grep 'Errno\|-110@0\|usage: main' $basedir/logdump.txt > /dev/null; then
    echo errors found!
    exit 1
fi
echo Check passed!

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
        make > /dev/null 2>&1 || ret=32
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
        echo release/${appname}svr -d -m ./mpsvr
        release/${appname}svr ./mpsvr
        sleep 2
        echo release/${appname}cli -m ./mp
        release/${appname}cli ./mp
        popd
        sleep 5
        python3 ./mainsvr.py fs/mpsvr/$svcpt 0 &
        sleep 3
        python3 ./maincli.py fs/mp/connection_0/$svcpt 0 || ret=37
        echo pkill -f mainsvr.py
        pkill -f mainsvr.py
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

