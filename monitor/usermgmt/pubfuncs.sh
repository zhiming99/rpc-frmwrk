#!/bin/bash

function backup_registry()
{
    _regname=$1
    _backup=$2
    if [ -z $_regname ] || [ -z $_backup ]; then
        echo Error missing parameters
        return 22
    fi
    if [ ! -f $_regname ]; then
        echo Error "$_regname" is not a valid file
        return 2
    fi
    if [ -f $_backup ]; then
        echo will erase the content of "$_backup", 'continue(Y/n)'?
        read answer
        if [ "x$answer" == "xy" ] || ["x$answer" == "xyes" ] || [ "x$answer" == "x" ]; then
            >$_backup
        else
            exit 0
        fi
    fi
    if [ ! -d backdir_0x123 ]; then
        if ! mkdir backdir_0x123; then
            echo Error cannot create temporary directory for backup purpose.
            return $?
        fi
    fi

    if ! regfsmnt -d $_regname backdir_0x123; then
        return $?
    fi
    while ! mountpoint ./backdir_0x123; do
        sleep 1
    done

    pushd backdir_0x123 > /dev/null
    tar cf ../bktar123.tar --xattrs .
    popd > /dev/null
    umount backdir_0x123
    sleep 1

    regfsmnt -i $_backup
    if ! regfsmnt -d $_backup backdir_0x123; then
        return $?
    fi
    while ! mountpoint ./backdir_0x123; do
        sleep 1
    done

    pushd backdir_0x123 > /dev/null
    tar xf ../bktar123.tar
    popd > /dev/null
    umount backdir_0x123
    rm bktar123.tar
    rm -rf backdir_0x123
    echo completed with backup file "$_backup"
}

function is_dir_empty()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing path to test
        return 1
    fi
    if [[ ! -d $1 ]]; then
        return 1
    fi
    if [ -z "$( ls -A $1 )" ]; then
        return 0
    else
        return 1 
    fi
}

function check_user_mount()
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
    if [ ! -f $base/usereg.dat ]; then
        echo "Error, did not find the user registry file."
        echo "you may want to use 'inituser.sh' to initialize one first"
        return 1
    fi
    if (( $mt == 1 ));then
        rootdir=`echo $appmp | awk '{print $3}'`
        if [ -d "$rootdir/usereg/users" ]; then
            rootdir="$rootdir/usereg"
            #echo find mount at $rootdir...
            return 0
        fi
    elif (( $mt == 0 ));then
        mp+=" invalidpath"
        for rootdir in $mp; do
            if [ -d "$rootdir/users" ]; then
                #echo find mount at $rootdir...
                return 0
            fi
        done
    fi
    mt=2
    rootdir="$base/mprpcfaddu"
    if [ ! -d $rootdir ]; then
        mkdir -p $rootdir
    fi
    if ! regfsmnt -d $base/usereg.dat $rootdir; then
        echo "Error, failed to mount usereg.dat"
        return 1
    fi

    while ! mountpoint $rootdir > /dev/null ; do
        sleep 1
    done
    echo mounted usereg.dat...
}

function join_group()
{
# join_group <group name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or group name
        return 1
    fi

    _group=$1
    _uname=$2
    _udir=./users/$_uname
    _gdir=./groups/$_group

    if [ ! -f $_udir/uid ] ; then
        echo user "$_uname" is not valid
        return 1
    fi

    if [ ! -f $_gdir/gid ] ; then
        echo group "$_group" does not exist
        return 1
    fi

    if [ ! -d $_udir/groups ]; then
        mkdir -p $_udir/groups
    fi

    if [ ! -d $_gdir/users ]; then
        return 1
    fi

    # link the user to the group's users dir
    # and link the group to the user's groups dir
    _gidval=`python3 $updattr -v ./groups/$_group/gid`
    _uidval=`python3 $updattr -v ./users/$_uname/uid`

    echo setup links to groups
    touch $_udir/groups/$_gidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_group\"}" $_udir/groups/$_gidval > /dev/null
    echo $_group > $_udir/groups/$_gidval

    touch $_gdir/users/$_uidval
    _file=$_gdir/users/$_uidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_uname\"}" $_file > /dev/null
    echo $_uname > $_file

    touch ./uids/$_uidval
    _file=./uids/$_uidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_uname\"}" $_file > /dev/null
    echo $_uname > $_file
}

function leave_group()
{
# leave_group <group name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or group name
        return 1
    fi

    _group=$1
    _uname=$2
    _udir=./users/$_uname
    _gdir=./groups/$_group

    if [ ! -d $_udir ]; then
        echo Error invalid user name
        return 1
    fi

    if [ "$_group" == "default" ]; then
        echo Error cannot leave default group
    fi

    if [ ! -d $_gdir ]; then
        echo Error invalid group name
        return 1
    fi

    if [ ! -d $_udir/groups ]; then
        echo Error cannot find groups joined
    fi

    if [ ! -d $_gdir/users ]; then
        echo Error cannot find users joined $_group
    fi

    # link the user to the group's users dir
    # and link the group to the user's groups dir
    _gidval=`python3 $updattr -v ./groups/$_group/gid`
    _uidval=`python3 $updattr -v ./users/$_uname/uid`

    rm $_udir/groups/$_gidval
    rm $_gdir/users/$_uidval
    rm ./uids/$_uidval
}

function add_group()
{
#this function assume the current directory is the root dir of the user registry
#and it requires one parameter: the group name
    echo adding group $1
    _group=$1
    if [ -d ./groups/$_group ]; then
        return 1
    fi

    mkdir -p ./groups/$_group/users
    if ! touch ./groups/$_group/gid; then
        echo Error failed to create  groups/$_group/gid $?
    fi

    _gidval=`python3 ${updattr} -a 'user.regfs' 1 ./gidcount`
    python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$_gidval}" ./groups/$_group/gid > /dev/null
    echo $_gidval > ./groups/$_group/gid
    touch ./gids/$_gidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_group\"}" ./gids/$_gidval > /dev/null
    echo $_group > ./gids/$_gidval

    touch ./groups/$_group/date
    datestr=$(date)
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$datestr\"}" ./groups/$_group/date > /dev/null
    echo $datestr > ./groups/$_group/date
    join_group $_group admin
}

function assoc_krb5user()
{
#usage assoc_krbuser <krb5user name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or kerberos user name
        return 1
    fi

    _krb5user=$1
    _uname=$2
    _udir=./users/$_uname

    if [ ! -f $_udir/uid ]; then
        echo Error user $_uname is not valid
        exit 1
    fi

    if [ ! -d $_udir/krb5users ];then
        mkdir $_udir/krb5users
    fi
    touch $_udir/krb5users/$_krb5user
    touch ./krb5users/$_krb5user
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_uname\"}" ./krb5users/$_krb5user > /dev/null
    echo $_uname > ./krb5users/$_krb5user
    return 0
}

function assoc_oa2user()
{
#usage assoc_oa2user <OAuth2 name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or kerberos user name
        return 1
    fi

    _oa2user=$1
    _uname=$2
    _udir=./users/$_uname

    if [ ! -f $_udir/uid ]; then
        echo Error user $_uname is not valid
        exit 1
    fi

    if [ ! -d $_udir/oa2users ];then
        mkdir $_udir/oa2users
    fi
    touch $_udir/oa2users/$_oa2user
    touch ./oa2users/$_oa2user
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_uname\"}" ./oa2users/$_oa2user > /dev/null
    echo $_uname > ./oa2users/$_oa2user
    return 0
}

function unassoc_krb5user()
{
#usage unassoc_krbuser <krb5user name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or kerberos user name
        return 1
    fi

    _krb5user=$1
    _uname=$2
    _udir=./users/$_uname

    if [ ! -d $_udir/krb5users ];then
        echo Error no krb5 users associated
        return 1
    fi

    if [ ! -f $_udir/krb5users/$_krb5user ];then
        echo Error no krb5 user '$_krb5user' associated
        return 1
    fi
    if [ -f ./krb5users/$_krb5user ]; then
       rm ./krb5users/$_krb5user
    fi
    rm $_udir/krb5users/$_krb5user
    return 0
}

function unassoc_oa2user()
{
#usage assoc_oa2user <OAuth2 name> <user name>
    if [ "x$1" == "x" ] || [ "x$2" == "x" ]; then
        echo Error missing user name or oa2user user name
        return 1
    fi

    _oa2user=$1
    _uname=$2
    _udir=./users/$_uname

    if [ ! -d $_udir/oa2users ];then
        echo Error no OAuth2 users associated
        return 1
    fi
    if [ ! -f $_udir/oa2users/$_oa2user ];then
        echo Error no OAuth2 user '$_oa2user' associated
        return 1
    fi
    if [ -f ./oa2users/$_oa2user ]; then
       rm ./oa2users/$_oa2user
    fi
    rm $_udir/oa2users/$_oa2user
    return 0
}

function list_groups_user()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi
    _gdir=./users/$_uname/groups
    if is_dir_empty $_gdir; then
        return 0
    fi
    for i in $_gdir/*; do
        _gname=`cat $i`
        echo $_gname $i
    done
}

function list_users_group()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing group name
        return 1
    fi
    _gname=$1
    _udir=./groups/$_gname/users
    if is_dir_empty $_udir; then
        return 0
    fi
    for i in $_udir/*; do
        _uname=`cat $i`
        echo $_uname $i
    done
}

function list_users()
{
    _udir=./users
    if is_dir_empty $_udir; then
        echo None
        return 0
    fi
    pushd $_udir > /dev/null
    for i in *; do
        _uidval=`python3 ${updattr} -v $i/uid`
        echo $i:$_uidval
    done
    popd > /dev/null
}

function list_groups()
{
    _gdir=./groups
    if is_dir_empty $_gdir; then
        echo no groups
        return 0
    fi
    pushd $_gdir > /dev/null
    for i in *; do
        _gidval=`python3 ${updattr} -v $i/gid`
        echo $i:$_gidval
    done
    popd > /dev/null
}

function disable_user()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi
    _uname=$1
    _udir=./users/$_uname
    touch $_udir/disabled
}

function enable_user()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi
    _uname=$1
    _udir=./users/$_uname
    if [ ! -f $_udir/disabled ]; then
        echo user $_uname already enabled
        return 0
    fi
    rm $_udir/disabled
}

function disable_group()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing group name
        return 1
    fi
    _group=$1
    _gdir=./groups/$_group
    touch $_gdir/disabled
}

function enable_group()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing group name
        return 1
    fi
    _group=$1
    _gdir=./groups/$_group
    if [ ! -f $_gdir/disabled ]; then
        echo group $_group already enabled
        return 0
    fi
    rm -f $_gdir/disabled
}

function remove_user()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi
    _uname=$1
    _udir=./users/$_uname

    _uidval=`python3 $updattr -v $_udir/uid`

    if ! is_dir_empty $_udir/groups; then
        for i in $_udir/groups/*; do
            _group=`python3 $updattr -v $i`
            rm ./groups/$_group/users/$_uidval
        done
    fi

    rm -f ./uids/$_uidval
    if [ -d $_udir/krb5users ] && ! is_dir_empty $_udir/krb5users; then
        for i in $_udir/krb5users/*; do
            _krb5user=`basename $i`
            rm ./krb5users/$_krb5user
        done
    fi

    if [ -d $_udir/oa2users ] && ! is_dir_empty $_udir/oa2users; then
        for i in $_udir/oa2users/*; do
            _oa2user=`basename $i`
            rm ./oa2users/$_oa2user
        done
    fi

    rm -rf $_udir
    echo removed user $_uname
}

function remove_group()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing group name
        return 1
    fi
    _group=$1
    _gdir=./groups/$_group

    if [ "x$_group" == "xdefault" ] || [ "x$_group" == "xadmin" ]; then
        echo Error cannot remove $_group
        exit 1
    fi

    _gidval=`python3 $updattr -v $_gdir/gid`
    if [ ! -d $_gdir/users ]; then
        rm -rf $_gdir
        echo group $_group is removed
        return 0
    fi

    if ! is_dir_empty $_gdir/users; then
        for i in $_gdir/users/*; do
            _uidval=`basename $i`
            _uname=`python3 $updattr -v ./uids/$_uidval`
            rm ./users/$_uname/groups/$_gidval
        done
    fi

    rm ./gids/$_gidval
    rm -rf $_gdir
    echo removed user $_group
}

function show_user()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi
    _uname=$1
    _udir=./users/$_uname
    if [ ! -d $_udir ]; then
        _uid=$1
        if [ ! -f ./uids/$_uid ]; then
            echo Error user $_uname does not exist
            return 1
        else
            _uname=`python3 $updattr -v ./uids/$_uid`
            _udir=./users/$_uname
        fi
    fi
    echo user name: $_uname
    echo uid: `python3 $updattr -v $_udir/uid`
    echo created on `cat $_udir/date`
    if [ -f $_udir/disabled ]; then
        echo "status: disabled"
    else
        echo "status: enabled"
    fi
    grouparr=
    if ! is_dir_empty $_udir/groups; then
        _arr=( $_udir/groups/* )
        _count=${#_arr[@]}
        _index=0
        for i in "${_arr[@]}"; do
            grouparr+=`python3 $updattr -v $i`
            if (( _index < _count-1));then
                grouparr+=", "
            fi
            ((_index++))
        done
        echo groups joined: $grouparr
    else
        echo groups joined: none
    fi

    grouparr=
    if [ -d $_udir/krb5users ] && ! is_dir_empty $_udir/krb5users; then
        _arr=( $_udir/krb5users/* )
        _count=${#_arr[@]}
        _index=0
        for i in "${_arr[@]}"; do
            _kname=`basename $i`
            grouparr+=$_kname
            if (( _index < _count-1));then
                grouparr+=", "
            fi
            ((_index++))
        done
    fi
    if [ "x$grouparr" != "x" ]; then
        echo krb5users: $grouparr
    fi

    grouparr=
    if [ -d $_udir/oa2users ] && ! is_dir_empty $_udir/oa2users; then
        _arr=( $_udir/oa2users/* )
        _count=${#_arr[@]}
        _index=0
        for i in "${_arr[@]}"; do
            _oname=`basename $i`
            grouparr+=$_oname
            if (( _index < _count-1));then
                grouparr+=", "
            fi
            ((_index++))
        done
    fi
    if [ "x$grouparr" != "x" ]; then
        echo oa2users: $grouparr
    fi
    
    if [ -f $_udir/passwd ]; then
        echo password: set
    else
        echo password: not set
    fi
}

function show_group()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing group name
        return 1
    fi
    _group=$1
    _gdir=./groups/$_group

    if [ ! -d $_gdir ]; then
        _gid=$1
        if [ ! -f ./gids/$_gid ]; then
            echo Error group $_group does not exist
            return 1
        else
            _group=`python3 $updattr -v ./gids/$_gid`
            _gdir=./groups/$_group
        fi
    fi
    _gidval=`python3 $updattr -v $_gdir/gid`
    echo group name: $_group
    echo gid: $_gidval

    if [ ! -d $_gdir/users ]; then
        return 0
    fi

    if is_dir_empty $_gdir/users; then
        echo member\(s\): none
        return 0
    fi
    _users=
    _arr=( $_gdir/users/* )
    _count=${#_arr[@]}
    _index=0
    for _uidval in "${_arr[@]}"; do
        _uname=`python3 $updattr -v $_uidval`
        _users+=$_uname
        if (( _index < _count-1 ));then
            _users+=", "
        fi
        ((_index++))
    done
    echo member\(s\): $_users
    return 0
}

function show_krb5_assoc()
{
    if is_dir_empty ./krb5users; then
        return 0
    fi
    for i in ./krb5users/*; do
        _krb5user=`basename $i`
        _uname=`python3 $updattr -v $i`
        echo $_krb5user "--->" $_uname
    done
}

function show_oauth2_assoc()
{
    if is_dir_empty ./oa2users; then
        return 0
    fi
    for i in ./oa2users/*; do
        _oa2user=`basename $i`
        _uname=`python3 $updattr -v $i`
        echo $_oa2user "--->" $_uname
    done
}

function set_password()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi

    _uname=$1 
    _udir=./users/$_uname

    if [ ! -f $_udir/uid ]; then
        echo Error user $_uname is not valid
        return 1
    fi

    touch $_udir/passwd
    echo -n Password:
    read -s password
    echo
    echo -n Enter again:
    read -s password1
    echo
    if [ "$password1" != "$password" ]; then
        echo Error the password was not enterred correctly
        echo you can change the password later with rpcfmodu
        return 1
    fi
    #salt=`tr -dc A-Za-z0-9 </dev/urandom | head -c 13; echo`
    salt=`echo -n $_uname | sha256sum | awk '{print $1}' | tr '[:lower:]' '[:upper:]'`
    firsthash=`echo -n $password | sha256sum | awk '{print $1}' | tr '[:lower:]' '[:upper:]'`
    passhash=`echo -n $firsthash$salt | sha256sum | awk '{print $1}' | tr '[:lower:]' '[:upper:]'`
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$passhash\"}" $_udir/passwd > /dev/null 
    passhash=
}

function clear_password()
{
    if [ "x$1" == "x" ] ; then
        echo Error missing user name
        return 1
    fi

    _uname=$1 
    _udir=./users/$_uname

    if [ ! -d $_udir/uid ]; then
        echo Error user $_uname is not valid
        return 1
    fi
    rm -f $_udir/passwd
}
