#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="fh"

function Usage()
{
cat << EOF
Usage: $0 [-fh] <app name> <point name> <attr name> <data type> <value>
    This command add an attribute <app name>/<point name>/<attr name> with
    <value> of <data type>
    -f: the <value> is a path to the value file. it is only valid if <data type>
    is blob or bytearray
    -h: Print this help
EOF
}

bFile=0
while getopts $OPTIONS opt; do
    case "$opt" in
    f) bFile=1
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

if (($bFile==0)); then
    set_attr_value $1 $2 $3 "$(jsonval $4 $5)" $4
else
    set_attr_blob $1 $2 $3 $5 $4
fi

popd > /dev/null
if (( $mt == 2 )); then
    fusermount3 -u $rootdir
fi

