#!/bin/bash
keydir=$(pwd)/tools/testcfgs/gmsslcfgs

pushd ./tools
input=$keydir/$1
output=${input%%.in}

# driver.json for bridge
python3 ./updgmskey.py $input $keydir > $output

# driver.json for reqfwdr

echo pidof rpcrouter
pgrep rpcrouter

pkill rpcrouter
pkill -f mainsvr.py
pkill -f maincli.py

cat $output

if which sudo; then
    SUDO="sudo"
else
    SUDO=
fi

$SUDO python3 /usr/local/bin/rpcf/rpcfgnui.py $output

popd

if [ ! -d reqfwdr ]; then
    mkdir reqfwdr
fi
if [ -f "$HOME/.rpcf/etc/rpcf/driver.json" ]; then
    drvPath="$HOME/.rpcf/etc/rpcf/driver.json"
else
    drvPath=/usr/local/etc/rpcf/driver.json 
fi

python3 ./tools/updgmskey.py $drvPath $keydir > reqfwdr/driver.json


