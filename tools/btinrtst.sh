#!/bin/bash
pushd examples/cpp/testypes

echo testing ROS RPC with builtin router
cat cmdline
bash cmdline
make
release/TestTypessvr -d
release/TestTypescli
pkill TestTypessvr
popd
