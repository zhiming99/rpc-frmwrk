#!/bin/bash
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh
addu=${script_dir}/rpcfaddu.sh
modu=${script_dir}/rpcfmodu.sh

if ! which regfsmnt; then
    echo cannot find program 'regfsmnt'. You may want \
        to install rpc-frmwrk first
    exit 1
fi
OPTIONS="sh" 
function Usage()
{
cat << EOF
Usage: $0 [-sh]
    This command initialize the application registry.
    -h: Print this help
    -s: format the user registry silently
EOF
}

bSilent=0
while getopts $OPTIONS opt; do
    case "$opt" in
    s)  bSilent=1
        ;;
    h)
        Usage
        exit 0
        ;;
    \?)
        echo $OPTARG >&2
        Usage
        exit 1
        ;;
    esac
done

pushd ${HOME} > /dev/null
if [ ! -d .rpcf ]; then mkdir .rpcf; fi
cd .rpcf
if [ ! -d ./testmnt ]; then mkdir ./testmnt; fi
if [[ -f usereg.dat ]] && (( $bSilent == 0 )); then
   echo "you are going to format the user registry, continue ( y/n )?(default: yes)"
   read answer
   if [ "x$answer" == "xy" ] || [ "x$answer" == "xyes" ] || [ "x$answer" == "x" ]; then
       mv usereg.dat usereg.dat.bak
   else
       exit 0
   fi
fi

regfsmnt -i ./usereg.dat || exit $?

regfsmnt -d ./usereg.dat ./testmnt
ticks=10
while ! mountpoint ./testmnt > /dev/null ; do
    echo waiting fs ready...
    sleep 1
    (( ticks-- ))
    if (( $ticks == 0 )); then
        echo Error failed to mount appreg.dat
        exit 110
    fi
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
if ! mountpoint ./testmnt > /dev/null;then
    rmdir ./testmnt
fi
popd > /dev/null

echo adding user 'admin'
if (( $bSilent==0 )); then
    bash $addu -p admin
else
    bash $addu -p admin <<< "123"
    echo -e "\033[0;33madmin's password is set to '123', and don't forget to change the weak password.\033[0m"
fi
bash $modu -g admin admin
