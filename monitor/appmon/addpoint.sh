#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <app name> <point name> <point type> <data type>
    This command add a point <point name> to <app name> with <point type>(
    output, input, setpoint ) and <data type> ( int, string, float, double, blob )
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

if [[  "x$@" == "x" ]]; then
   echo "Error invalid parameters"
   Usage
   exit 1
fi

                                                                                                  
if ! which regfsmnt > /dev/null; then
    echo cannot find program 'regfsmnt'. You may want \
        to install rpc-frmwrk first
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
    if ! grant_perm ./apps/$1/points 0007 202; then
        ret = 2
        break
    fi
    add_point $1 $2 $3 $4
    break
done
if (( ret <= 1 )); then
    restore_perm 201
fi
if (( ret <= 2 )); then
    restore_perm 202
fi

popd > /dev/null
if (( $mt == 2 )); then
    umount $rootdir
fi
