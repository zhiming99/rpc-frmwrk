#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh
  
OPTIONS="h"

function Usage()
{
cat << EOF
Usage: $0 [-h] <group name> 
    this command remove the group <group name>
    -h: print this help
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

if [  "x$@" == "x" ]; then
   echo "Error group name is not specified"
   Usage
   exit 1
fi

source $pubfuncs
check_user_mount

pushd $rootdir
echo start remove groups $@ ...
for group in "$@"; do
    remove_user $group
done
popd


