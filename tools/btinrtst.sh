#!/bin/bash
auth=
if [ "x$1" == "x-a" ]; then
    auth=$1
fi
pkill rpcrouter
pushd examples/cpp/testypes
tail -n 18 maincli.cpp
head -n 22 TestTypesSvcsvr.cpp

echo testing ROS RPC with builtin router
cat cmdline
bash cmdline
make
sleep 1
release/TestTypessvr $auth &
sleep 2
release/TestTypescli $auth --driver ./driver-cli.json
pkill TestTypessvr
popd
