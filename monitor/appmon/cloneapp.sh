#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  

function Usage()
{
cat << EOF
Usage: $0 <app name> <clone name>
    This command clone an application <app name> to a new application <clone name>.
    It is meant to be a start point for further customization efficiently.
    -h: Print this help
EOF
}
     
if ! which regfsmnt > /dev/null; then
    echo cannot find program 'regfsmnt'. You may want \
        to install rpc-frmwrk first
    exit 1
fi

source $appfuncs

if ! check_appreg_mount; then
    exit $?
fi


ret=0

cmdline="cloneapp <appName> <clone name>"
if [[ $# -lt 2 ]]; then
    echo Error missing parameters
    echo $cmdline
    if (( $mt == 2 )); then
        fusermount3 -u $rootdir
    fi
    exit 1
fi

srcapp=$1
dstapp=$2

echo cloning application $srcapp to $dstapp
if [[ "$srcapp" == "appmonsvr1" ]]; then
    echo cannot clone appmonsvr1
    if (( $mt == 2 )); then
        fusermount3 -u $rootdir
    fi
    exit 1
fi
pushd $rootdir > /dev/null
add_stdapp $dstapp
if [ $? -eq 0 ]; then
    for point in ./apps/$srcapp/points/* ; do
        ptName=$(basename $point)
        ptype=$(cat $point/ptype)
        dtype=$(get_attr_value $srcapp $ptName datatype i)
        dtype=$(type2str $dtype)
        if [[ ! -d ./apps/$dstapp/points/$ptName ]]; then
            add_point $dstapp $ptName $ptype $dtype 
        fi

        echo cloning $dstapp/$ptName...
        if [[ "$ptype" != "setpoint" ]] || [[ "$ptName" == "display_name" ]]; then
            true
        else
            cp -f --preserve=all $point/value ./apps/$dstapp/points/$ptName/value
        fi

        if ! is_dir_empty ./apps/$srcapp/points/$ptName; then
            for attr in ./apps/$srcapp/points/$ptName/*; do
                if [[ ! -f $attr ]]; then
                    continue
                fi
                attrName=$(basename $attr)
                if [[ "$attrName" == "ptype" ]] ||
                    [[ "$attrName" == "dtype" ]] ||
                    [[ "$attrName" == "value" ]] ; then
                    continue
                fi
                if [[ "$attrName" == "logcount" ]] ||
                    [[ "$attrName" == "ptrcount" ]]; then
                    set_attr_value $dstapp $ptName $attrName "$(jsonval i 0)" i
                else
                    cp -f --preserve=all $attr ./apps/$dstapp/points/$ptName/$attrName 
                fi
            done
        fi
        if [[ "$ptype" == "input" ]] || [[ "$ptype" == "output" ]]; then
            ptrDir="./apps/$srcapp/points/$ptName/ptrs"
            if ! is_dir_empty $ptrDir; then
                for ptrFile in $ptrDir/*;do
                    ptr="$(cat $ptrFile)"
                    IFS='/' read -ra comps <<< "$ptr"
                    peerApp=${comps[0]}
                    peerPoint=${comps[1]}
                    if [[ "$ptype" == "input" ]]; then
                        add_link $peerApp $peerPoint $dstapp $ptName
                    elif [[ "$ptype" == "output" ]]; then
                        add_link $dstapp $ptName $peerApp $peerPoint
                    fi
                done
            fi
            logDir=./apps/$srcapp/points/$ptName/logptrs 
            if [[ -d $logDir ]] &&
               ( ! is_dir_empty $logDir ) ; then
                for ptrFile in $logDir/*; do
                    ptr=$(cat $ptrFile)
                    IFS='/' read -ra comps <<< "$ptr"
                    peerApp=${comps[0]}
                    peerPoint=${comps[1]}
                    add_log_link $dstapp $ptName $peerApp $peerPoint
                done
            fi
        fi
    done
else
    echo return value $ret
fi

popd > /dev/null
if (( $mt == 2 )); then
    fusermount3 -u $rootdir
fi

exit $ret
