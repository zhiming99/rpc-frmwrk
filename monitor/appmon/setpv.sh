#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-hf] <app name> <point name> <data type> <value>
    This command set value of <data type> to <app name>/<point name>
    -f: the <value> is a file whose content is the value. The <data type> must be 'blob' or 'bytearray'
    -h: Print this help
EOF
}
bFile=0
while getopts $OPTIONS opt; do
    case "$opt" in
    f)  bFile=1
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

shift $((OPTIND-1))

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

appname=$1
ptname=$2
dt=$3
val=$4
if [ -z $appname ] || [ -z $ptname ] || [ -z $dt ] || [ -z "$val" ]; then
    echo Error input parameter
    Usage
    exit 22
fi
if ((bFile==1)); then
    if [ "$dt" != "blob" ] && [ "$dt" != "bytearray" ]; then
        echo Error '-f' is only valid for 'blob' or 'bytearray'
        exit 22
    fi
    if [ ! -f $val ]; then
        echo Error "$val" is not a valid file
        exit 22
    fi

    fileSize=`stat -c %s $val`
    if (( fileSize >= 8192 * 1024 ));then
        echo Error file is beyond 8MB, too big to store
        exit 7
    fi
fi

dtnum=$(str2type $dt)
ptpath=./apps/$appname/points/$ptname
if [ ! -d $ptpath ]; then
    echo Error cannot find the point.
    exit 22
fi
if (( $dtnum <= 7 ));then
    set_point_value $appname $ptname "$(jsonval $dt $val)" $dt
elif (( bFile == 0 )); then
    set_point_value $appname $ptname "$(jsonval $dt $val)" $dt
else
    set_large_point_value $appname $ptname $val $dt
fi


popd > /dev/null
if (( $mt == 2 )); then
    umount $rootdir
fi

