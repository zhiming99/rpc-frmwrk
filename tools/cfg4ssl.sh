#!/bin/bash
keydir=$(pwd)/tools/testcfgs
pushd ./tools/testcfgs
sed "s:XXXXX:${keydir}:g" sslcfg.json.in > sslcfg.json
echo pidof rpcrouter
pidof rpcrouter
sudo kill -9 `pidof rpcrouter` || true
sudo kill -9 `pidof python3` || true
cat sslcfg.json
sudo python3 /usr/local/bin/rpcf/rpcfgnui.py sslcfg.json
popd
