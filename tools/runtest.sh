lib_dir=/usr/local/lib
bin_dir=/usr/local/bin
export LD_LIBRARY_PATH=${lib_dir}:${lib_dir}/rpcf
cat /proc/cpuinfo
if [ "$1" == "2" ]; then
    echo starting the app server...
    ${bin_dir}/rpcf/ifsvrsmk &
    ${bin_dir}/rpcf/kasvrtst &
    ${bin_dir}/rpcf/actcsvrtst &
    ${bin_dir}/rpcf/prsvrtst &
    echo starting the bridge...
    ${bin_dir}/rpcrouter -r 2 
elif [ "$1" == "1" ]; then
    echo starting the reqfwdr...
    ${bin_dir}/rpcrouter -r 1 &
    sleep 15
    echo starting echo test...
    ${bin_dir}/rpcf/ifclismk || exit 1
    ${bin_dir}/rpcf/kaclitst || exit 2
    ${bin_dir}/rpcf/actclitst || exit 3
    ${bin_dir}/rpcf/prclitst || exit 4
    kill -9 `pidof rpcrouter`
    ${bin_dir}/rpcf/btinrtsvr &
    sleep 5
    ${bin_dir}/rpcf/btinrtcli || exit 5
    kill -9 `pidof btinrtsvr`
    kill -9 `pidof ifsvrsmk`
    kill -9 `pidof kasvrtst`
    kill -9 `pidof actcsvrtst`
    kill -9 `pidof prsvrtst`
    echo restart rpcrouters...
    ${bin_dir}/rpcrouter -r 2&
    ${bin_dir}/rpcrouter -r 1&
    pwd
    pushd ./ridl
    ${bin_dir}/ridlc -I ./ -pO ./pytest fulltest.ridl || exit 6
    cd ./pytest
    make
    python3 mainsvr.py &
    sleep 5
    echo start the Python testcases
    python3 maincli.py || exit 7
    popd
fi
