#!/bin/bash
keydir=$(pwd)/tools/testcfgs
pushd ./tools/testcfgs
sed "s:XXXXX:${keydir}:g" sslcfg.json.in > sslcfg.json
kill -9 `pidof rpcrouter` || true
kill -9 `pidof python3` || kill -9 `pidof python` || true
cat sslcfg.json
sudo python3 /usr/local/bin/rpcf/rpcfgnui.py sslcfg.json
popd
