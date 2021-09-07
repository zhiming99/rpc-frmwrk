#!/bin/bash
DBUS_INC=$(echo $(pkg-config --cflags dbus-1 jsoncpp) | sed 's/^-I/, "/g;s/ -I/", "/g;s/\(.*\)$/\1"/' | sed 's:/:\\/:g')
sed "s:XXXXXXXX:$DBUS_INC:g" pyproject.toml.tmpl > pyproject.toml
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
sed -i "s:YYY:$scriptDir/..:g" pyproject.toml
if [ "x$1" \> "x" ]; then
    sed -i "s:XXXWLRPATH:-Wl,-rpath=$1,-rpath=$1/rpcf:" pyproject.toml
fi
if [ "x${ARMBUILD}" == "x1" ]; then
    sed -i "s:\(\"combase\):\"atomic\", \1:" pyproject.toml
fi
