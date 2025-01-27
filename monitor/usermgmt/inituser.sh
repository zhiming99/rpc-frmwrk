#!/bin/bash
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh

if ! which regfsmnt; then
    echo cannot find program 'regfsmnt'. You may want \
        to install rpc-frmwrk first
    exit 1
fi

pushd ${HOME} > /dev/null
if [ ! -d .rpcf ]; then mkdir .rpcf; fi
cd .rpcf
if [ ! -d ./testmnt ]; then mkdir ./testmnt; fi
if [ ! -f usereg.dat ]; then
   touch usereg.dat;
else
   echo "you are going to format the user registry, continue ( y/n )?(default: yes)"
   read answer
   if [ "x$answer" == "xy" ] || ["x$answer" == "xyes" ] || [ "x$answer" == "x" ]; then
       mv usereg.dat usereg.dat.bak
       regfsmnt -i ./usereg.dat || exit $?
   else
       exit 0
    fi
fi

regfsmnt -d ./usereg.dat ./testmnt
while ! mountpoint ./testmnt > /dev/null ; do
    echo waiting fs ready...
    sleep 1
done


cd ./testmnt
touch uidcount
if ! setfattr -n 'user.regfs' -v '{ "t": 3, "v":10000 }' ./uidcount > /dev/null; then
    exit $?
fi
if ! getfattr -n 'user.regfs' ./uidcount ; then
    exit $?
fi

touch gidcount
if ! setfattr -n 'user.regfs' -v '{ "t": 3, "v":80000 }' ./gidcount > /dev/null; then
    exit $?
fi
if ! getfattr -n 'user.regfs' ./gidcount; then
    exit $?
fi

source $pubfuncs

if [ ! -d users ]; then
    mkdir -p users groups krb5users oa2users uids gids
    add_group "admin"
    add_group "default"
fi
echo initialization completed.
cd ..
fusermount3 -u ./testmnt
rmdir ./testmnt
popd > /dev/null
