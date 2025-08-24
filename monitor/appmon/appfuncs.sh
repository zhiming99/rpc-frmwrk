#!/bin/bash

_script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
_pubfuncs=${_script_dir}/pubfuncs.sh
rpcfshow=${_script_dir}/rpcfshow.sh
showapp=${_script_dir}/showapp.sh
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

function undo_check_appreg_mount()
{
    base=$HOME/.rpcf
    if (( $mt==2 )); then
        umount ${rootdir}
        rmdir ${rootdir}
    fi
}

function check_appreg_mount()
{
#this function export 'mt' 'base' and 'rootdir'
# mt is the mount type
# base is the base directory of the rpcf config directory
# rootdir is the mount point of the usereg.dat
    mp=`mount | grep '^regfsmnt' | awk '{print $3}'`
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
    if [ ! -d $base ]; then
        echo "Error, the rpc-frmwrk is not configed properly."
        echo "Please install rpc-frmwrk or config rpc-frmwrk with rpcfg.py"
        return 1
    fi
    if [ ! -f $base/appreg.dat ]; then
        echo "Error, cannot find the app registry file."
        echo "you may want to use 'initappreg.sh' command to format the app registry file"
        return 1
    fi
    if (( $mt == 1 ));then
        rootdir=`echo $appmp | awk '{print $3}'`
        if [ -d "$rootdir/appreg/apps" ]; then
            rootdir="$rootdir/appreg"
            #echo find mount at $rootdir...
            return 0
        fi
    elif (( $mt == 0 ));then
        # there could be several mount point
        mp+=" invalidpath"
        for rootdir in $mp; do
            if [ -d "$rootdir/apps" ]; then
                #echo find mount at $rootdir...
                return 0
            fi
        done
    fi
    mt=2
    rootdir="$base/mprpcfappreg"
    if [ ! -d $rootdir ]; then
        mkdir -p $rootdir
    fi
    if ! regfsmnt -d $base/appreg.dat $rootdir; then
        echo "Error, failed to mount appreg.dat"
        return 1
    fi

    while ! mountpoint $rootdir > /dev/null ; do
        sleep 1
    done
    #echo mounted appreg.dat...
    return 0
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
    w | word) echo 2
        ;;
    i | int) echo 3
        ;;
    q | qword) echo 4
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
        echo Error str2type unknown data type $1>&2
        return 22
        ;;
    esac
}

function type2str()
{
    if [ -z $1 ];then
        echo Error missing parameters
        return 22
    fi
    case "$1" in
    1) echo byte
        ;;
    2) echo word 
        ;;
    3) echo int
        ;;
    4) echo qword
        ;;
    5) echo float
        ;;
    6) echo double
        ;;
    7) echo string
        ;;
    9) echo object
        ;;
    10) echo bytearray
        ;;
    *)
        echo Error type2str unknown data type $1>&2
        return 22
        ;;
    esac
}

function type2size()
{
    if [ -z $1 ];then
        echo Error missing parameters
        return 22
    fi
    case "$1" in
    1) echo 1
        ;;
    2) echo 2 
        ;;
    3) echo 4
        ;;
    4) echo 8
        ;;
    5) echo 4
        ;;
    6) echo 8
        ;;
    *)
        echo Error type2str unknown data type $1>&2
        return 22
        ;;
    esac
}

function jsonval()
{
    _t=$1
    shift
    _v="$@"
    if [ -z $_t ] || [ -z "$_v" ];then
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
    "w" | "word") _t=2
        ;;
    "i" | "int") _t=3
        ;;
    "q" | "qword") _t=4
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
        echo -n "$_v"
        return 0
        ;;
    *)
        echo Error jsonval unknown data type $_t>&2
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
    touch point_flags

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
    python3 $updattr -u 'user.regfs' "$(jsonval i 0 )" point_flags > /dev/null

    if [ $_pttype != 'setpoint' ]; then
        mkdir ptrs || true
        touch ptrcount
        if ! setfattr -n 'user.regfs' -v '{ "t": 3, "v":0 }' ./ptrcount > /dev/null; then
            return $?
        fi
    fi
    popd > /dev/null
}

function set_point_value()
{
# $1 app name 
# $2 point name
# $3 point value
# $4 datatype
    _appname=$1
    _ptname=$2
    _value="$3"
    _dt=$4
    if set_attr_value $_appname $_ptname value "$_value" $_dt; then
        _ptpath=./apps/$_appname/points/$_ptname
        _dtnum=$(str2type $_dt)
        python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$_dtnum}" $_ptpath/datatype > /dev/null
    fi
    return $?
}

function set_large_point_value()
{
# $1 app name 
# $2 point name
# $3 point value file
# $4 datatype
    _appname=$1
    _ptname=$2
    _value="$3"
    _dt=$4
    if set_attr_blob $_appname $_ptname value "$_value" $_dt; then
        _ptpath=./apps/$_appname/points/$_ptname
        _dtnum=$(str2type $_dt)
        python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$_dtnum}" $_ptpath/datatype > /dev/null
    fi
    return $?
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
    _ptype=$(python3 $updattr -v $_outpath/ptype)
    if (($_ptype != 0 )); then
        echo Output point is not 'output' type
        return 22
    fi
    _ptype=$(python3 $updattr -v $_inpath/ptype)
    if (($_ptype != 1 )); then
        echo input point is not 'input' type
        return 22
    fi

    linkin="$_inapp/$_inpt"
    linkout="$_outapp/$_outpt"
    inid=`python3 ${updattr} -a 'user.regfs' 1 $_inpath/ptrcount`
    outid=`python3 ${updattr} -a 'user.regfs' 1 $_outpath/ptrcount`

    if [ ! -d $_inpath/ptrs ]; then
        mkdir $_inpath/ptrs
    fi
    chmod o+w $_inpath/ptrs
    echo $linkout > $_inpath/ptrs/ptr$inid
    fileSize=`stat -c %s $_inpath/ptrs/ptr$inid`
    if (( $fileSize == 0 )); then
        echo failed to write to $_inpath/ptrs/ptr$inid
    fi
    len=${#linkout}
    if (( len < 95 ));then
        python3 ${updattr} -u 'user.regfs' "$(jsonval s $linkout)" $_inpath/ptrs/ptr$inid > /dev/null
    fi
    chmod o-w $_inpath/ptrs

    if [ ! -d $_outpath/ptrs ]; then
        mkdir $_outpath/ptrs
    fi
    chmod o+w $_outpath/ptrs
    echo $linkin > $_outpath/ptrs/ptr$outid
    fileSize=`stat -c %s $_outpath/ptrs/ptr$outid`
    if (( $fileSize == 0 )); then
        echo failed to write to $_outpath/ptrs/ptr$outid
    fi
    len=${#linkin}
    if (( len < 95 ));then
        python3 ${updattr} -u 'user.regfs' "$(jsonval s $linkin)" $_outpath/ptrs/ptr$outid > /dev/null
    fi
    chmod o-w $_outpath/ptrs
}

function set_attr_value()
{
# #1 app name 
# $2 point name
# $3 attr name
# $4 attr value
# $5 datatype
    _appname=$1
    _ptname=$2
    _attr=$3
    _value="$4"
    _dt=$5

    if [ -z $_appname ] || [ -z $_ptname ] || [ -z "$_value" ] || [ -z $_dt ] || [ -z $_attr ];then
        echo Error invalid set_attr_value parameters
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_ptname 
    if [ ! -d $_ptpath ]; then
        echo Error set_attr_value point "$_ptpath" not exist
        return 2
    fi
    _dtnum=$(str2type $_dt)
    if [ -z $_dtnum ]; then
        echo Error bad data type $_dt@$_appname/$_ptname
        return 22
    fi
    if (( $_dtnum <= 7 ));then
        if [ ! -f $_ptpath/$_attr ]; then
            touch $_ptpath/$_attr
        fi
        python3 $updattr -u 'user.regfs' "$_value" $_ptpath/$_attr > /dev/null
    else
        echo "$_value" > $_ptpath/$_attr
    fi
    return $?
}

function get_attr_value()
{
# #1 app name 
# $2 point name
# $3 attr name
    _appname=$1
    _ptname=$2
    _attr=$3
    _dt=$4

    if [ -z $_appname ] || [ -z $_ptname ] || [ -z $_attr ] || [ -z $_dt ];then
        echo Error invalid get_attr_value parameters
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_ptname 
    if [ ! -d $_ptpath ]; then
        echo Error set_attr_value point "$_ptpath" not exist
        return 2
    fi
    _dtnum=$(str2type $_dt)
    if [ -z $_dtnum ]; then
        echo Error bad data type $_dt@$_appname/$_ptname
        return 22
    fi
    if (( $_dtnum <= 7 ));then
        python3 $updattr -v $_ptpath/$_attr > /dev/null
    else
        cat $_ptpath/$_attr
    fi
    return $?
}

function set_attr_blob()
{
# #1 app name 
# $2 point name
# $3 attr name
# $4 attr file
# $5 datatype
    _appname=$1
    _ptname=$2
    _attr=$3
    _file=$4
    _dt=$5

    if [ -z $_appname ] || [ -z $_ptname ] || [ -z "$_file" ] || [ -z $_dt ] || [ -z $_attr ];then
        echo Error set_attr_blob invalid parameters
        return 22
    fi
    if [ ! -f $_file ]; then
        echo Error set_attr_blob invalid value file
        return -2
    fi
    _ptpath=./apps/$_appname/points/$_ptname 
    if [ ! -d $_ptpath ]; then
        echo Error set_attr_blob point "$_ptpath" not exist
        return 2
    fi
    _dtnum=$(str2type $_dt)
    if [[ -z $_dtnum ]] || (( $_dtnum <= 7 )) ; then
        echo Error set_attr_blob bad data type $_dt@$_appname/$_ptname
        return 22
    fi
    cat $_file > $_ptpath/$_attr
    return $?
}

function rm_attribute()
{
# #1 app name 
# $2 point name
# $3 attr name
# $4 attr value
# $5 datatype
    _appname=$1
    _ptname=$2
    _attr=$3

    if [ -z $_appname ] || [ -z $_ptname ] || [ -z $_attr ]; then
        echo Error invalid set_attr_value parameters
        return 22
    fi

    arrAttrBase=("value" "datatype" "unit" "ptype" "ptrcount" )
    if [[ " ${arrAttrBase[*]} " =~ [[:space:]]${_attr}[[:space:]] ]]; then
        echo Error point "$_attr" is baseline point, and cannot be removed
        return 13
    fi

    _ptpath=./apps/$_appname/points/$_ptname/$_attr 
    if [ ! -f $_ptpath ]; then
        return 0
    fi
    rm -f $_ptpath
    return $?
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
    _ptrpath="./apps/$_appname/points/$_ptname/ptrs"
    if [ ! -d $_ptrpath ]; then
        echo Error internal error
        return 2
    fi
    if is_dir_empty $_ptrpath; then 
        echo there is no link to delete for "$_appname:$_ptname"
    else
        link2="$_appname2/$_ptname2"
        chmod o+w $_ptrpath
        pushd $_ptrpath > /dev/null
        for i in *; do
            _peerlink=`python3 $updattr -v $i`
            if [ -z $_peerlink ]; then
                continue
            fi
            if [[ "$_peerlink" != "$link2" ]]; then
                continue
            fi
            rm $i
            #python3 ${updattr} -a 'user.regfs' -1 ../ptrcount > /dev/null
            break
        done
        popd > /dev/null
        chmod o-w $_ptrpath
    fi

    link="$_appname/$_ptname"
    _ptrpath2="./apps/$_appname2/points/$_ptname2/ptrs"
    if [ ! -d $_ptrpath2 ]; then
        echo Error internal error
        return 2
    fi
    if is_dir_empty $_ptrpath2; then 
        echo there is no link to delete for "$_appname2:$_ptname2"
        return 0
    else
        chmod o+w $_ptrpath2
        pushd $_ptrpath2 > /dev/null
        for i in *; do
            _peerlink=`python3 $updattr -v $i`
            if [ -z $_peerlink ]; then
                continue
            fi
            if [[ "$_peerlink" != "$link" ]]; then
                continue
            fi
            rm $i
            #python3 ${updattr} -a 'user.regfs' -1 ../ptrcount > /dev/null
            break
        done
        popd > /dev/null
        chmod o-w $_ptrpath2
    fi
    return 0
}

function rm_point_nocheck()
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
        echo Error point "$__ptpath" not exist
        return 2
    fi
    pushd $__ptpath > /dev/null
    if [ ! -f setpoint ] && [ -d ptrs ]; then
        if ! is_dir_empty ptrs; then
            cd ptrs
            for i in *; do
                __peerlink=`cat $i`
                if [ -z $__peerlink ]; then
                    continue
                fi
                __peerapp=`awk -F'/' '{print $1}' $i`
                __peerpt=`awk -F'/' '{print $2}' $i`
                pushd $__curdir > /dev/null
                rm_link $__appname $__ptname $__peerapp $__peerpt
                popd > /dev/null
            done
            cd ..
        fi
    fi
    if [[ -d logptrs ]]; then
        if ! is_dir_empty logptrs; then
            if [[ -d logs ]]; then
                __user="true"
            else
                __user="false"
            fi
            cd logptrs
            for i in *; do
                __peerlink=`cat $i`
                if [ -z $__peerlink ]; then
                    continue
                fi
                __peerapp=`awk -F'/' '{print $1}' $i`
                __peerpt=`awk -F'/' '{print $2}' $i`
                pushd $__curdir > /dev/null
                if [[ "$__user" == "true" ]]; then
                    rm_log_link $__appname $__ptname $__peerapp $__peerpt
                else
                    rm_log_link $__peerapp $__peerpt $__appname $__ptname
                fi
                popd > /dev/null
            done
            cd ..
        fi
    fi
    cd ..
    rm -rf $__ptname
    popd > /dev/null
    return 0
}

function rm_point()
{
    __curdir=`pwd`
    __appname=$1
    __ptname=$2

    arrBasePts=( "rpt_timer" "obj_count" "max_qps" "cur_qps"
    "max_streams_per_session" "pending_tasks" "restart" "cmdline" "pid" "working_dir" "uptime" 
    "rx_bytes" "tx_bytes" "failure_count" "resp_count" "req_count" "vmsize_kb"
    "cpu_load" "open_files" "conn_count" "offline_times" "rx_bytes_total"
    "tx_bytes_total" "uptime_total" "online_notify" "offline_notify" )

    if [[ " ${arrBasePts[*]} " =~ [[:space:]]${__ptname}[[:space:]] ]]; then
        echo Error point "$__ptname" is baseline point, and cannot be removed
        return 13
    fi
    rm_point_nocheck $1 $2
    return $?
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
    change_application_mode o+rwx $_appname
    pushd ./apps/$_appname/points > /dev/null
    for i in *; do
        pushd $_curdir > /dev/null
        rm_point_nocheck $_appname $i
        popd > /dev/null
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
        owner_stream=`python3 $updattr -v ./$i/owner_stream`
        if [[ -z "$owner_stream" ]] || (( $owner_stream == 0 )); then
            echo "$i: offline"
        else
            echo "$i: online"
        fi
    done
    popd > /dev/null
}

function show_point()
{
    _pt=$1
    _dt=$(python3 $updattr -v $_pt/datatype)
    _dtname=$(type2str $_dt)
    _output=$_pt":"
    _mode=`ls -l $_pt/value|awk '{print $1}'`
    _output+=" "$_mode
    _output+=" "$(cat $_pt/ptype)" "$_dtname" val="
    if (( $_dt <= 7 )); then
        _output+="`python3 $updattr -v $_pt/value 2>/dev/null`"
    else
        _fileSize=`stat -c %s $i`
        if (( $_fileSize > 0 )); then
            _output+="`cat $_pt/value`"
        else
            _output+="None"
        fi
    fi
    echo "$_output"
}

function show_links()
{
    if is_dir_empty ptrs; then
        echo links: None
        return 0
    fi
    pushd ptrs > /dev/null
    echo link:
    for i in *; do
        if (( _ptype==0 )); then
            echo -e '\t' $_appname/$_pt '-->' `python3 $updattr -v $i`
        else
            echo -e '\t' `python3 $updattr -v $i` '-->' $_appname/$_pt
        fi
    done
    popd > /dev/null
}

function show_point_detail()
{
    _appname=$1
    _pt=$2
    if [[ -z $_appname ]] || [[ -z $_pt ]]; then
        echo Error show_point_detail missing application/point name
        return 22
    fi
    _ptpath=./apps/$_appname/points/$_pt
    if is_dir_empty $_ptpath; then
        echo Error the point content is empty
        return 2
    fi
    _dt=$(python3 $updattr -v $_ptpath/datatype)
    _dtname=$(type2str $_dt)
    _output=$_appname/$_pt":"
    _mode=`ls -l $_ptpath/value|awk '{print $1}'`
    _output+=" "$_mode
    _output+=" "$(cat $_ptpath/ptype)" "$_dtname" val="
    if (( $_dt <= 7 )); then
        _output+="`python3 $updattr -v $_ptpath/value 2>/dev/null`"
    else
        abc="$(cat $_ptpath/value)"
        _output+="$abc"
    fi
    _ptype=$(python3 $updattr -v $_ptpath/ptype)
    echo "$_output"
    pushd $_ptpath > /dev/null
    for i in *; do
        if [[ "$i" == "value" ]] || [[ "$i" == "datatype" ]]; then
            continue
        fi
        if [ ! -f $i ]; then
            continue
        fi
        _val="$(python3 $updattr -v $i)"
        if [ -z "$_val" ]; then
            fileSize=`stat -c %s $i`
            if (( $fileSize > 0 )); then
                _val="`cat $i`"
            else
                _val="None"
            fi
        fi
        echo -e '\t' $i: "$_val"
    done
    if [[ -d ptrs ]]; then
        show_links $1 $2
    fi
    if [[ -d logptrs ]];then
        show_log_links
    fi

    popd > /dev/null
}

function print_owner()
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
    _uid=`ls -ld $_appdir | awk '{print $3}'`
    _gid=`ls -ld $_appdir | awk '{print $4}'`

    _uname=`bash $rpcfshow -u $_uid | grep 'user name' | awk '{print $3}'`
    _gname=`bash $rpcfshow -g $_gid | grep 'group name' | awk '{print $3}'`
    echo "owner: $_uname $_uid"
    echo "group: $_gname $_gid"
}

function list_points()
{
    _appname=$1
    if [ -z $_appname ];then
        echo Error missing application name
        return 22
    fi
    if ! print_owner $_appname; then
        return $?
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
    pushd $_appdir/points > /dev/null
    for i in *; do
        show_point $i
    done
    popd > /dev/null
}

function get_app_owner()
{
    _instname=$1
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi
    print_owner $_instname | grep "owner:" | awk '{print $2}'
}

function get_app_group()
{
    _instname=$1
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi
    print_owner $_instname | grep "group:" | awk '{print $2}'
}

function get_app_uid()
{
    _instname=$1
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi
    print_owner $_instname | grep "owner:" | awk '{print $3}'
}

function get_app_gid()
{
    _instname=$1
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi
    print_owner $_instname | grep "group:" | awk '{print $3}'
}

function change_application_owner()
{
    _instname=$1
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi

    _user="$2"
    _group="$3"

    if [ -z $_user ]; then
        _uid=`get_app_uid $_instname`
    fi
    if [ -z $_group ]; then
        _gid=`get_app_gid $_instname`
    fi
    if [ -z $_uid ]; then
        _uid=`bash $rpcfshow -u $_user | grep 'uid:' | awk '{print $2}'`
    fi
    if [ -z $_gid ]; then
        _gid=`bash $rpcfshow -g $_group | grep 'gid:' | awk '{print $2}'`
    fi
    if [ -z $_uid ] || [ -z $_gid ]; then
        echo "Error the given user/group not valid"
        return 22
    fi

    chown -R $_uid:$_gid ./apps/$_instname
    return $?
}

function change_application_mode()
{
    _instname=$2
    if [ ! -d ./apps/$_instname ]; then
        echo Error application $_instname does not exist
        return 1
    fi

    _mode=$1
    if [ -z $_mode ]; then
        echo Error empty mode string
    fi

    chmod -R $_mode ./apps/$_instname
    return $?
}

function init_log_file()
{
    if [[ "x$1" == "x" ]]; then
        return 0
    fi
    typeid=3
    if [[ "x$2" != "x" ]]; then
        typeid=$(str2type $2)
    fi
    typesize=$(type2size $typeid)
    output=$1
    magic=0x706c6f67
    counter=0
    recsize=$(expr 4 + $typesize)
    reserved=0

    python3 -c "import sys; sys.stdout.buffer.write((${magic}).to_bytes(4, 'big')); sys.stdout.buffer.write((${counter}).to_bytes(4, 'big')); sys.stdout.buffer.write((${typeid}).to_bytes(2, 'big')); sys.stdout.buffer.write((${recsize}).to_bytes(2, 'big')); sys.stdout.buffer.write((${reserved}).to_bytes(4, 'big')) " > $output
}

function add_log_link()
{
# $1 user-app
# $2 user point to log
# $3 logger-app
# $4 logger point
    _userapp=$1
    _userpt=$2
    _logapp=$3
    _logpt=$4
    _logfile=$5

    if [[ "x" == "x$username" ]]; then
        username="admin"
    fi
    if [[ "x" == "x$groupname" ]] then
        groupname="admin";
    fi
    _uid=`bash $rpcfshow -u $username | grep 'uid:' | awk '{print $2}'`
    _gid=`bash $rpcfshow -g $groupname | grep 'gid:' | awk '{print $2}'`

    if [[ "x$_uid" == "x" ]] || [[ "x$_gid" == "x" ]]; then
        echo "Error cannot find the specified user/group"
        return 22;
    fi

    if [ -z $_userapp ] || [ -z $_userpt ] || [ -z $_logapp ] || [ -z $_logpt ];then
        echo Error missing parameters
        return 22
    fi
    _logpath="./apps/$_logapp/points/$_logpt"
    _userpath="./apps/$_userapp/points/$_userpt"
    if [ ! -d $_userpath ] ||
       [ ! -d $_logpath ]; then
        echo Error cannot find the pair of points to link
        return 22
    fi

    chmod o+w $_logpath
    chmod o+w $_userpath

    # setup pointer on log point

    if [[ ! -f $_logpath/logcount ]]; then
        touch $_logpath/logcount;
        python3 ${updattr} -u 'user.regfs' "$(jsonval i 0)" $_logpath/logcount > /dev/null
    fi
    logid=`python3 ${updattr} -a 'user.regfs' 1 $_logpath/logcount`

    if [[ ! -f $_userpath/logcount ]]; then
        touch $_userpath/logcount;
        python3 ${updattr} -u 'user.regfs' "$(jsonval i 0)" $_userpath/logcount > /dev/null
    fi
    userid=`python3 ${updattr} -a 'user.regfs' 1 $_userpath/logcount`

    if [[ ! -d $_logpath/logptrs ]]; then
        mkdir $_logpath/logptrs
    fi

    loglink="$_logapp/$_logpt/ptr$logid"
    userlink="$_userapp/$_userpt/ptr$userid"

    bexist=0
    chmod o+w $_logpath/logptrs
    if ! is_dir_empty $_logpath/logptrs; then
        #check if the ptr is already present
        for i in $_logpath/logptrs/*; do
            _peerlink=`python3 $updattr -v $i`
            if [[ -z $_peerlink ]]; then
                _peerlink=`cat $i`
                if [[ -z $_peerlink ]]; then
                    continue
                fi
            fi
            if [[ "$(dirname $_peerlink)" == "$_userapp/$_userpt" ]]; then
                bexist=1
                echo found duplicated user pointer
                echo pointer to $_userapp/$_userpt already exists
                break
            fi
        done
    fi
    if (($bexist==0)); then
        echo create userlink $userlink
        echo $userlink > $_logpath/logptrs/ptr$logid
        fileSize=`stat -c %s $_logpath/logptrs/ptr$logid`
        if (( $fileSize == 0 )); then
            echo failed to write to $_logpath/logptrs/ptr$logid
        fi
        len=${#userlink}
        if (( len < 95 ));then
            python3 ${updattr} -u 'user.regfs' "$(jsonval s $userlink)" $_logpath/logptrs/ptr$logid > /dev/null
        fi
    fi

    # setup pointer on user point

    if [[ ! -d $_userpath/logptrs ]]; then
        mkdir $_userpath/logptrs
    fi
    chmod o+w $_userpath/logptrs

    if ! is_dir_empty $_userpath/logptrs; then
        #check if the ptr is already present
        for i in $_userpath/logptrs/*; do
            _peerlink=`python3 $updattr -v $i`
            if [[ -z $_peerlink ]]; then
                _peerlink=`cat $i`
                if [[ -z $_peerlink ]]; then
                    continue
                fi
            fi
            if [[ "$(dirname $_peerlink)" == "$_logapp/$_logpt" ]]; then
                bexist=1
                echo pointer to $_logapp/$_logpt already exists
                break
            fi
        done
    fi
    if (($bexist==0)); then
        echo create loglink $loglink
        echo $loglink > $_userpath/logptrs/ptr$userid
        fileSize=`stat -c %s $_userpath/logptrs/ptr$userid`
        if (( $fileSize == 0 )); then
            echo failed to write to $_userpath/logptrs/ptr$userid
        fi
        len=${#loglink}
        if (( len < 95 ));then
            python3 ${updattr} -u 'user.regfs' "$(jsonval s $loglink)" $_userpath/logptrs/ptr$userid > /dev/null
        fi
        if [[ -d $_userpath/logs ]]; then
            mkdir $_userpath/logs
        fi
        dt=`python3 $updattr -v $_userpath/datatype`
        typestr=$(type2str $dt)
        if [[ "x" == "x$_logfile" ]]; then
            touch $_userpath/logs/ptr$userid-0
            init_log_file $_userpath/logs/ptr$userid-0 $typestr
        else
            touch $_userpath/logs/ptr$userid-extfile
            init_log_file $_logfile $typestr
            echo $_logfile > $_userpath/logs/ptr$userid-extfile
        fi

        touch $_userpath/average
        python3 $updattr -u 'user.regfs' "$(jsonval d 0.0)" $_userpath/average > /dev/null
    fi

    chmod o-w $_logpath/logptrs
    chmod o-w $_userpath/logptrs
    chmod o-w $_userpath/logs
    chmod o-w $_logpath
    chmod o-w $_userpath

    chown -R $_uid:$_gid $_logpath/logptrs 
    chown -R $_uid:$_gid $_logpath/logcount
    chown -R $_uid:$_gid $_userpath/logptrs
    chown -R $_uid:$_gid $_userpath/logs
    chown -R $_uid:$_gid $_userpath/logcount
}

function rm_log_link()
{
# $1 user app
# $2 user point
# $3 logger app
# $4 logger point
    _userapp=$1
    _userpt=$2
    _logapp=$3
    _logpt=$4
    if [ -z $_userapp ] || [ -z $_userpt ] 
       [ -z $_logapp ] || [ -z $_logpt ] ;then
        echo Error missing parameters
        return 22
    fi
    _ptrpath="./apps/$_userapp/points/$_userpt/logptrs"
    if [ ! -d $_ptrpath ]; then
        echo $_ptrpath does not exist
    elif is_dir_empty $_ptrpath; then 
        echo there is no link to delete for "$_userapp:$_userpt"
    else
        link2="$_logapp/$_logpt"
        chmod o+w $_ptrpath
        pushd $_ptrpath > /dev/null
        for i in *; do
            _peerlink=`python3 $updattr -v $i`
            if [[ -z $_peerlink ]]; then
                _peerlink=`cat $i`
                if [[ -z $_peerlink ]]; then
                    continue
                fi
            fi
            _peerlink=$(dirname $_peerlink)
            if [[ "$_peerlink" != "$link2" ]]; then
                continue
            fi
            rm -f $i

            echo "you are going to remove the point logs $(dirname $_ptrpath)/logs/$i-*, continue ( y/N )?(default: No)"
            read answer
            if [ "x$answer" == "xn" ] || [ "x$answer" == "xno" ] || [ "x$answer" == "x" ]; then
                echo the logs is kept.
                break;
            fi
            if [[ -f ../logs/$i-0 ]] || [[ -f ../logs/$i-extfile ]]; then
                rm -f ../logs/$i-*
            fi
            #python3 ${updattr} -a 'user.regfs' -1 ../logcount > /dev/null
        done
        popd > /dev/null
        chmod o-w $_ptrpath
    fi

    link="$_userapp/$_userpt"
    _ptrpath2="./apps/$_logapp/points/$_logpt/logptrs"
    if [ ! -d $_ptrpath2 ]; then
        echo Error internal error
        return 2
    fi
    if is_dir_empty $_ptrpath2; then 
        echo there is no link to delete for "$_logapp:$_logpt"
        return 0
    else
        chmod o+w $_ptrpath2
        pushd $_ptrpath2 > /dev/null
        for i in *; do
            _peerlink=`python3 $updattr -v $i`
            if [[ -z $_peerlink ]]; then
                _peerlink=`cat $i`
                if [[ -z $_peerlink ]]; then
                    continue
                fi
            fi
            _peerlink=$(dirname $_peerlink)
            if [[ "$_peerlink" != "$link" ]]; then
                continue
            fi
            rm -f $i
            #python3 ${updattr} -a 'user.regfs' -1 ../logcount > /dev/null
        done
        popd > /dev/null
        chmod o-w $_ptrpath2
    fi
    return 0
}

function show_log_links()
{
    if is_dir_empty logptrs; then
        echo logs: None
        return 0
    fi

    if [[ -d logs ]]; then
        direction="-->"
    else
        direction="<--"
    fi
    pushd logptrs > /dev/null
    echo logs:
    for i in *; do
        echo -e '\t' $_appname/$_pt "${direction}" $(dirname `python3 $updattr -v $i`)
    done
    popd > /dev/null
}
# add a standard app : add_stdapp <app inst name> [<owner> <group>]
function add_stdapp()
{
    _instname=$1
    if [[ -d ./apps/$_instname ]]; then
        echo Error application $_instname already exist
        return 1
    fi

    _user="$2"
    _group="$3"

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
        return 22
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
    set_attr_value $_instname rpt_timer unit "$(jsonval 'i' 0 )" i
    add_point $_instname obj_count output i
    set_attr_value $_instname obj_count value "$(jsonval 'i' 0 )" i
    add_point $_instname max_qps setpoint i
    set_attr_value $_instname max_qps value "$(jsonval 'i' 0 )" i
    add_point $_instname cur_qps output i
    set_attr_value $_instname cur_qps value "$(jsonval 'i' 0 )" i
    add_point $_instname max_streams_per_session setpoint i
    set_attr_value $_instname max_streams_per_session value "$(jsonval 'i' 0 )" i
    add_point $_instname pending_tasks output i
    set_attr_value $_instname pending_tasks value "$(jsonval 'i' 0 )" i
    add_point $_instname restart input i
    set_attr_value $_instname restart pulse "$(jsonval 'i' 1 )" i
    add_point $_instname cmdline setpoint blob
    add_point $_instname pid output i 
    add_point $_instname working_dir  setpoint blob

    add_point $_instname rx_bytes output qword
    set_attr_value $_instname rx_bytes value "$(jsonval 'q' 0 )" q
    add_point $_instname tx_bytes output qword
    set_attr_value $_instname tx_bytes value "$(jsonval 'q' 0 )" q
    add_point $_instname failure_count output i
    set_attr_value $_instname failure_count value "$(jsonval 'i' 0 )" i
    add_point $_instname resp_count output i
    set_attr_value $_instname resp_count value "$(jsonval 'i' 0 )" i
    add_point $_instname req_count output i
    set_attr_value $_instname req_count value "$(jsonval 'i' 0 )" i
    add_point $_instname vmsize_kb  output q
    add_point $_instname cpu_load  output f
    add_point $_instname open_files  output i
    set_attr_value $_instname open_files value "$(jsonval 'i' 0 )" i
    add_point $_instname conn_count  output i
    set_attr_value $_instname conn_count value "$(jsonval 'i' 0 )" i

    add_point $_instname uptime output i
    set_attr_value $_instname uptime unit "$(jsonval 's' 'sec' )" s

    #online_notify sends a json object
    add_point $_instname online_notify output s
    #flags 1 : no storaage for this point
    set_attr_value $_instname online_notify point_flags "$(jsonval 'i' 1 )" i
    #online_notify sends a json object
    add_point $_instname offline_notify output s
    set_attr_value $_instname offline_notify point_flags "$(jsonval 'i' 1 )" i

    add_point $_instname offline_times setpoint i
    set_attr_value $_instname offline_times value "$(jsonval 'i' 0 )" i
    add_point $_instname uptime_total setpoint i
    set_attr_value $_instname uptime_total value "$(jsonval 'i' 0 )" i
    add_point $_instname rx_bytes_total setpoint qword
    set_attr_value $_instname rx_bytes_total value "$(jsonval 'q' 0 )" q
    add_point $_instname tx_bytes_total setpoint qword
    set_attr_value $_instname tx_bytes_total value "$(jsonval 'q' 0 )" q

    chown $_uid:$_gid -R ./apps/$_instname
    find ./apps/$_instname -type f -exec chmod ug+rw,o+r '{}' ';'
    find ./apps/$_instname -type d -exec chmod ug+rwx,o+rx '{}' ';'
    chmod -R o-rwx ./apps/$_instname/points/restart
    chmod -R o+w ./apps/$_instname/notify_streams
    if [[ "x$_instname" == "xtimer1" ]]; then
        return 0
    fi
    add_link timer1 clock1 $_instname rpt_timer
    add_link $_instname offline_notify timer1 offline_action
    return 0
}
