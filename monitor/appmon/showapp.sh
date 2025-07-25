#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <app name>
    This command show detail information about the application <app name> from the app registry.
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
   echo "Error app name is not specified"
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

for appname in "$@"; do
    if [[ ! -d ./apps/$appname ]]; then
        echo $appname does not exist
        continue
    fi
    owner_stream=`python3 $updattr -v ./apps/$appname/owner_stream`
    if [[ -z "$owner_stream" ]] || (( $owner_stream == 0 )); then
        echo status: offline
    else
        echo "status: online"
        echo "stream: $owner_stream"
    fi
    if ! list_points $appname; then
        break
    fi
done

popd > /dev/null
undo_check_appreg_mount

