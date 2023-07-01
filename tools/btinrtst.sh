#!/bin/bash
pkill rpcrouter
pushd examples/cpp/testypes
tail -n 18 maincli.cpp
head -n 22 TestTypesSvcsvr.cpp

echo testing ROS RPC with builtin router
cat cmdline
bash cmdline
make
sleep 1
release/TestTypessvr &
sleep 2
release/TestTypescli
pkill TestTypessvr
popd
