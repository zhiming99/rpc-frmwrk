#!/bin/bash

_script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
_pubfuncs=${script_dir}/pubfuncs.sh
source $_pubfuncs

function wait_mount()
{
    if [ -z $1 ] || [ -z $2 ]; then
        echo Error missing parameters
        return 22
    fi
    _mntdir=$1
    _ticks=$2
    while ! mountpoint $_mntdir > /dev/null ; do
        echo waiting fs ready...
        sleep 1
        (( _ticks-- ))
        if (( $_ticks == 0 )); then
            echo Error mount timedout @$_mntdir
            exit 110
        fi
    done
    return 0
}

function check_appreg_mount()
{
#this function export 'mt' 'base' and 'rootdir'
# mt is the mount type
# base is the base directory of the rpcf config directory
# rootdir is the mount point of the usereg.dat
    mp=`mount | grep regfsmnt`
    if [ "x$mp" == "x" ];then
        appmp=`mount | grep appmonsvr`
    fi
    mt=0
    if [ "x$mp" == "x" ] && [ "x$appmp" == "x" ]; then
        mt=2
    elif [ "x$appmp" != "x" ]; then
        mt=1
    else
        mt=0
    fi
    base=$HOME/.rpcf
    if [ ! -f $base/appreg.dat ]; then
        echo "Error, did not find the app registry file."
        echo "you may want to use 'initappreg.sh' command to format the app registry file"
        exit 1
    fi
    if (( $mt == 2 ));then
        rootdir="$base/mprpcfappreg"
        if [ ! -d $rootdir ]; then
            mkdir -p $rootdir
        fi
        if ! regfsmnt -d $base/appreg.dat $rootdir; then
            echo "Error, failed to mount appreg.dat"
            exit 1
        fi

        while ! mountpoint $rootdir > /dev/null ; do
            sleep 1
        done
        echo mounted appreg.dat...
    elif (( $mt == 1 ));then
        rootdir=`echo $appmp | awk '{print $3}'`
        echo find mount at $rootdir...
        if [ ! -d "$rootdir/appreg/apps" ]; then
            echo "Error cannot find the mount point of app registry"
            exit 1
        fi
        rootdir="$rootdir/appreg"
    elif (( $mt == 0 ));then
        rootdir=`echo $mp | awk '{print $3}'`
        echo find mount at $rootdir...
        if [ ! -d "$rootdir/apps" ]; then
            echo "Error cannot find the mount point of app registry"
            exit 1
        fi
    fi
}

function str2type()
{
    if [ -z $1 ];then
        echo Error missing parameters
        return 22
    fi
    case "$1" in
    bool) echo 1
        ;;
    b | byte) echo 1
        ;;
    i | int) echo 3
        ;;
    f | float) echo 5
        ;;
    d | double) echo 6
        ;;
    s | string) echo 7
        ;;
    o | object) echo 9
        ;;
    blob | bytearray) echo 10
        ;;
    *)
        echo Error unknown data type>&2
        return 22
        ;;
    esac
}

function jsonval()
{
    _t=$1
    _v=$2
    if [ -z $1 ] || [ -z $2 ];then
        echo Error missing parameters
        return 22
    fi
    case "$_t" in
    "bool") _t=1
        if [ "$_v" == "true" ]; then
            _v=1
        elif [ "$_v" == "false" ]; then
            _v=0
        else
            echo Error not a valid value for boolean value
            return 22
        fi
        ;;
    "b" | "byte") _t=1
        ;;
    "i" | "int") _t=3
        ;;
    "f" | "float") _t=5
        ;;
    "d" | "double") _t=6
        ;;
    "s" | "string") _t=7
        echo -n "{\"t\":$_t,\"v\":\"$_v\"}"
        return 0
        ;;
    "o" | "object") _t=9
        echo -n "{\"t\":$_t,\"v\":\"$_v\"}"
        return 0
        ;;
    "blob" | "bytearray") _t=10
        echo -n "{\"t\":$_t,\"v\":\"$_v\"}"
        return 0
        ;;
    *)
        echo Error unknown data type>&2
        return 22
        ;;
    esac
    echo -n "{\"t\":$_t,\"v\":$_v}"
    return 0
}

# all functions assuming current dir is the root dir of app registry
function add_application()
{
    _instname=$1
    mkdir -p ./apps/$_instname
    pushd ./apps/$_instname > /dev/null
    #owner stream is for input event from data producer
    touch owner_stream
    python3 $updattr -u 'user.regfs' "$(jsonval i 0)" owner_stream > /dev/null

    #notify_streams is for monitors
    mkdir points notify_streams

    touch birth_time
    echo $(date) > birth_time

    popd > /dev/null
}

function add_point()
{
# $1 app name
# $2 point name
# $3 point type
# $4 data type
    _appname=$1
    _ptname=$2
    _pttype=$3
    _datatype=$4
    if [ -z $_appname ] || [ -z $_pttype ] || [ -z $_ptname ] || [ -z $_datatype ];then
        echo Error missing parameters
        return 22
    fi
    if [ ! $_pttype == 'output' ] && [ ! $_pttype == 'input' ] &&
        [ ! $_pttype == 'setpoint' ]; then
        echo Error invalid point type
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_ptname
    if ! mkdir -p $_ptpath; then
        return $?
    fi
    pushd $_ptpath > /dev/null
    touch value
    touch datatype
    _dt=$(str2type $_datatype)
    python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$_dt}" datatype > /dev/null
    touch unit
    touch ptype

    if [ "$_pttype" == 'output' ]; then
        python3 $updattr -u 'user.regfs' "$(jsonval i 0 )" ptype > /dev/null
        echo output > ptype
    elif [ "$_pttype" == 'input' ]; then
        python3 $updattr -u 'user.regfs' "$(jsonval i 1 )" ptype > /dev/null
        echo input > ptype
    else
        python3 $updattr -u 'user.regfs' "$(jsonval i 2 )" ptype > /dev/null
        echo setpoint > ptype
    fi

    if [ $_pttype != 'setpoint' ]; then
        mkdir ptrs
        touch ptrcount
        if ! setfattr -n 'user.regfs' -v '{ "t": 3, "v":0 }' ./ptrcount > /dev/null; then
            exit $?
        fi
    fi
    popd > /dev/null
}

function set_point_value()
{
# #1 app name 
# $2 point name
# $3 point point value
    _appname=$1
    _ptname=$2
    _value=$3
    if [ -z $_appname ] || [ -z $_ptname ] || [ -z $_value ];then
        echo Error pointer parameters
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_ptname 
    if [ ! -d $_ptpath ]; then
        echo Error point "$_ptname" not exist
        return 2
    fi
    python3 $updattr -u 'user.regfs' "$_value" $_ptpath/value > /dev/null
}

function add_link()
{
# $1 output-app
# $2 ouput point 
# $3 input-app
# $4 input point
    _outapp=$1
    _outpt=$2
    _inapp=$3
    _inpt=$4
    if [ -z $_outapp ] || [ -z $_outpt ] || [ -z $_inapp ] || [ -z $_inpt ];then
        echo Error missing parameters
        return 22
    fi
    _inpath="./apps/$_inapp/points/$_inpt"
    _outpath="./apps/$_outapp/points/$_outpt"
    if [ ! -d $_outpath ] ||
       [ ! -d $_inpath ]; then
        echo Error 
        return 22
    fi
    linkin="$_inapp/$_inpt"
    linkout="$_outapp/$_outpt"
    inid=`python3 ${updattr} -a 'user.regfs' 1 $_inpath/ptrcount`
    outid=`python3 ${updattr} -a 'user.regfs' 1 $_outpath/ptrcount`

    echo $linkout > $_inpath/ptrs/ptr$inid
    len=${#linkout}
    if (( len < 95 ));then
        echo python3 ${updattr} -u 'user.regfs' "$(jsonval s $linkout)" $_inpath/ptrs/ptr$inid
        python3 ${updattr} -u 'user.regfs' "$(jsonval s $linkout)" $_inpath/ptrs/ptr$inid
    fi

    echo $linkin > $_outpath/ptrs/ptr$outid
    len=${#linkout}
    if (( len < 95 ));then
        python3 ${updattr} -u 'user.regfs' "$(jsonval s $linkin)" $_outpath/ptrs/ptr$outid
    fi
}

function set_attr_value()
{
# #1 app name 
# $2 point name
# $3 attr name
# $4 attr value
    _appname=$1
    _ptname=$2
    _attr=$3
    _value=$4
    if [ -z $_appname ] || [ -z $_ptname ] || [ -z $_attr ] || [ -z $_value ];then
        echo Error pointer parameters
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_ptname 
    if [ ! -d $_ptpath ]; then
        echo Error point "$_ptname" not exist
        return 2
    fi
    if [ ! -f $_ptpath/$_attr ]; then
        touch $_ptpath/$_attr
    fi
    python3 $updattr -u 'user.regfs' "$_value" $_ptpath/$_attr > /dev/null
}

function rm_link()
{
# $1 app name
# $2 point name
# $3 app name2
# $4 point name2
    _appname=$1
    _ptname=$2
    _appname2=$3
    _ptname2=$4
    if [ -z $_appname ] || [ -z $_ptname ] 
       [ -z $_appname2 ] || [ -z $_ptname2 ] ;then
        echo Error missing parameters
        return 22
    fi
    _ptrpath="./apps/$_appname/points/$_ptname/ptrs/"
    if [ ! -d $_ptrpath ]; then
        echo Error internal error
    fi
    if is_dir_empty $_ptrpath; then 
        echo there is no link to delete for "$_appname:$_ptname"
    fi

    link2="$_appname2/$_ptname2"

    pushd $_ptrpath > /dev/null
    for i in *; do
        _peerlink=`cat $i`
        if [ -z $_peerlink ]; then
            continue
        fi
        if [ "$_peerlink" != "$link2" ]; then
            continue
        fi
        rm -f $i
        break
    done
    popd > /dev/null

    link="$_appname/$_ptname"
    _ptrpath2="./apps/$_appname2/points/$_ptname2/ptrs/"
    if [ ! -d $_ptrpath2 ]; then
        echo Error internal error
    fi
    if is_dir_empty $_ptrpath2; then 
        echo there is no link to delete for "$_appname2:$_ptname2"
    fi

    pushd $_ptrpath2 > /dev/null
    for i in *; do
        _peerlink=`cat $i`
        if [ -z $_peerlink ]; then
            continue
        fi
        if [ "$_peerlink" != "$link" ]; then
            continue
        fi
        rm -f $i
        break
    done
    popd > /dev/null
}

function rm_point()
{
    __curdir=`pwd`
    __appname=$1
    __ptname=$2
    if [ -z $__appname ] || [ -z $__ptname ]; then
        echo Error missing parameters
        return 22
    fi
    __ptpath=./apps/$__appname/points/$__ptname 
    if [ ! -d $__ptpath ]; then
        echo Error point "$__ptname" not exist
        return 2
    fi
    pushd $__ptpath > /dev/null

    if [ ! -f setpoint ] && [ -d ptrs ]; then
        cd ptrs
        for i in *; do
            __peerlink=`cat $i`
            if [ -z $__peerlink ]; then
                continue
            fi
            __peerapp=`awk -F'/' '{print $1}'` $i
            __peerpt=`awk -F'/' '{print $2}'` $i
            pushd $__curdir > /dev/null
            rm_link $__appname $__ptname $__peerapp $__peerpt
            popd > /dev/null
        done
        cd ..
    fi
    cd ..
    rm -rf $__ptname
    popd > /dev/null
    return 0
}

function remove_application()
{
    _curdir=`pwd`
    _appname=$1
    if [ -z $_appname ] ;then
        echo Error missing application name
        return 22
    fi
    if [ ! -d ./apps/$_appname ]; then
        echo Error application "$_appname" is not valid
        return 2
    fi
    pushd ./apps/$_appname/points > /dev/null
    for i in *; do
        pushd $_curdir
        rm_point $_appname $i
        popd
    done
    popd > /dev/null
    rm -rf ./apps/$_appname
    return 0
}

function list_applications()
{
    if is_dir_empty ./apps; then
        echo None
        return 0
    fi
    pushd ./apps > /dev/null
    for i in *; do
        echo $i
    done
    popd > /dev/null
}

function list_points()
{
    _appname=$1
    if [ -z $_appname ];then
        echo Error missing application name
        return 22
    fi
    _appdir=./apps/$_appname
    if [ ! -d $_appdir ]; then
        echo Error invalid application "$_appname"
        return 22
    fi
    if is_dir_empty $_appdir; then
        echo None
        return 0
    fi
    pushd $_appdir > /dev/null
    for i in *; do
        echo $i
    done
    popd > /dev/null
}

# add a standard app : add_stdapp <app inst name> [<owner> <group>]
function add_stdapp()
{
    _instname=$1
    if [ -f ./apps/$_instname ]; then
        echo Error application $_instname already exist
        return 1
    fi

    $_user=$2
    $_group=$2

    if [ -z $_user ]; then
        _user='admin'
    fi
    if [ -z $_group ]; then
        _group='admin'
    fi

    _uid=`bash $rpcfshow -u $_user | grep 'uid:' | awk '{print $2}'`
    _gid=`bash $rpcfshow -g $_group | grep 'gid:' | awk '{print $2}'`
    if [ -z $_uid ] || [ -z $_gid ]; then
        echo "Error the given user/group not valid"
        exit 22
    fi

    mkdir -p ./apps/$_instname
    pushd ./apps/$_instname > /dev/null

    #owner stream is for input event from data producer
    touch owner_stream
    python3 $updattr -u 'user.regfs' "$(jsonval i 0)" owner_stream > /dev/null

    #notify_streams is for monitors
    mkdir points notify_streams

    touch birth_time
    echo $(date) > birth_time
    popd > /dev/null

    add_point $_instname rpt_timer input i
    set_attr_value $_instname rpt_timer unit "$(jsonval 'i' 0 )"
    add_point $_instname obj_count output i
    add_point $_instname max_rqs setpoint i
    add_point $_instname cur_rqs output i
    add_point $_instname max_streams_per_session setpoint i
    add_point $_instname pending_tasks  output i
    add_point $_instname restart input i
    set_attr_value $_instname restart pulse "$(jsonval 'i' 1 )"
    add_point $_instname cmdline setpoint blob
    add_point $_instname pid output i 
    add_point $_instname working_dir  setpoint blob

    add_point $_instname uptime output i
    set_attr_value $_instname uptime unit "$(jsonval 's' 'sec' )"

    chown $_uid:$_gid -R ./apps/$_instname
    find . -type f -exec chmod ug+rw,o+r '{}' ';'
    find . -type d -exec chmod ug+rwx,o+rx '{}' ';'
    chmod -R o-rwx ./apps/$_instname/points/restart
}
