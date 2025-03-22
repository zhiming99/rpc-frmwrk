#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
  
OPTIONS="u:g:h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <ugo+-=perm> <app name>
    This command change the owner and group of the application <app name>
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
modestr=$1

if [ -z "$modestr" ]; then
   echo "Error app name is not specified"
   Usage
   exit 1
fi

shift

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
    if ! change_application_mode $modestr $appname; then
        break
    fi
    echo owner/group changed
done

popd > /dev/null
if (( $mt == 2 )); then
    umount $rootdir
fi


