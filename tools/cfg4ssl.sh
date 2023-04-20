#!/bin/bash
keydir=$(pwd)/tools/testcfgs
pushd ./tools/testcfgs
input=$1
output=${input%.in}
sed "s:XXXXX:${keydir}:g" $1 > $output
echo pidof rpcrouter
pidof rpcrouter
if which sudo; then
    SUDO="sudo"
else
    SUDO=
fi
$SUDO pkill rpcrouter || true
$SUDO pkill python3 || true
cat $output
$SUDO python3 /usr/local/bin/rpcf/rpcfgnui.py $output
popd
