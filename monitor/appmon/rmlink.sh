#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <app1> <app1 point> <app2> <app2 point> 
    This command remove a link between two points if exist
    -h: Print this help
EOF
}
     
while getopts $OPTIONS opt; do
    case "$opt" in
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

shift $((OPTIND-1))

if [[ "x$@" == "x" ]]; then
   echo "Error invalid parameters"
   Usage
   exit 1
fi

if ! which regfsmnt > /dev/null; then
    echo cannot find program 'regfsmnt'. You may want to install rpc-frmwrk first
    exit 1
fi

source $appfuncs
if ! check_appreg_mount; then
    exit $?
fi

pushd $rootdir > /dev/null

while true; do
    if ! grant_perm ./apps/$1 0005 201; then
        ret = 1
        break
    fi
    if ! grant_perm ./apps/$1/points 0005 202; then
        ret = 2
        break
    fi
    if ! grant_perm ./apps/$1/points/$2 0005 203; then
        ret = 3
        break
    fi
    if ! grant_perm ./apps/$3 0005 204; then
        ret = 4
        break
    fi
    if ! grant_perm ./apps/$3/points 0005 205; then
        ret = 5
        break
    fi
    if ! grant_perm ./apps/$3/points/$4 0005 206; then
        ret = 6
        break
    fi
    rm_link $1 $2 $3 $4
    break
done
if (( ret <= 1 )); then
    restore_perm 201
fi
if (( ret <= 2 )); then
    restore_perm 202
fi
if (( ret <= 3 )); then
    restore_perm 203
fi
if (( ret <= 4 )); then
    restore_perm 204
fi
if (( ret <= 5 )); then
    restore_perm 205
fi
if (( ret <= 6 )); then
    restore_perm 206
fi

popd > /dev/null
if (( $mt == 2 )); then
    fusermount3 -u $rootdir
fi


