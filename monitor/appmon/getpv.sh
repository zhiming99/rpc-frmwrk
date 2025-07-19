#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <app name> <point name>
    This command get point value of <app name>/<point name>
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
if [ -z $appname ] || [ -z $ptname ]; then
   echo "Error app/point names are not specified"
    Usage
    popd > /dev/null
    undo_check_appreg_mount
    exit 22
fi

ptpath=./apps/$appname/points/$ptname
dtnum=`python3 $updattr -v $ptpath/datatype`

if [ ! -d $ptpath ]; then
    echo Error cannot find the point.
    popd > /dev/null
    undo_check_appreg_mount
    exit 22
fi

dtype=$(type2str $dtnum)
if (( $dtnum <= 7 ));then
    echo $dtype: `python3 $updattr -v $ptpath/value`
else
    echo $dtype: `cat $ptpath/value`
fi


popd > /dev/null
undo_check_appreg_mount
