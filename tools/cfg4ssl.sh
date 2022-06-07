#!/bin/bash
keydir=$(pwd)/tools/testcfgs
pushd ./tools/testcfgs
input=$1
output=${input%.in}
sed "s:XXXXX:${keydir}:g" $1 > $output
echo pidof rpcrouter
pidof rpcrouter
sudo kill -9 `pidof rpcrouter` || true
sudo kill -9 `pidof python3` || true
cat $output
sudo python3 /usr/local/bin/rpcf/rpcfgnui.py $output
popd
