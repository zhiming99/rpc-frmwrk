#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="u:g:h"

function Usage()
{
cat << EOF
Usage: $0 [-h] [-u <user name> ][-g <group name> ] <app name>
    This command change the owner and group of the application <app name>
    -h: Print this help
    -u: Set the owner of the application instances.
    -g: Set the group of the application instances.
EOF
}
     
while getopts $OPTIONS opt; do
    case "$opt" in
    u)  username=${OPTARG}
        ;;
    g)  groupname=${OPTARG}
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
    if [ -z $username ]; then
        username=`get_app_owner $appname`
    fi
    if [ -z $groupname ]; then
        groupname=`get_app_group $appname`
    fi
    if ! change_application_owner $appname $username $groupname; then
        break
    fi
    echo owner/group changed
done

popd > /dev/null
if (( $mt == 2 )); then
    fusermount3 -u $rootdir
fi

