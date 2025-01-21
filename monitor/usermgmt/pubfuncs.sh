#!/bin/bash
function check_user_mount()
{
#this function export 'mt' 'base' and 'rootdir'
# mt is the mount type
# base is the base directory of the rpcf config directory
# rootdir is the mount point of the usereg.dat
    mp=`mount | grep regfsmnt`
    if [ "x$mp" == "x" ];then
        appmp=`mount | grep appmnt`
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
        exit 1
    fi
    if (( $mt == 2 ));then
        rootdir="$base/mprpcfaddu"
        if [ -d $rootdir ]; then
            mkdir -p $rootdir
        fi
        if ! regfsmnt -d $base/usereg.dat $rootdir; then
            echo "Error, failed to mount usereg.dat"
            exit 1
        fi

        while ! mountpoint $rootdir > /dev/null ; do
            sleep 1
        done
        echo mounted usereg.dat...
    elif (( $mt == 1 ));then
        rootdir=`echo $appmp | awk '{print $3}'`
        if [ ! -d "$rootdir/usereg/users" ]; then
            echo "Error cannot find the mount point of user registry"
            exit 1
        fi
        rootdir="$rootdir/usereg"
    elif (( $mt == 0 ));then
        rootdir=`echo $mp | awk '{print $3}'`
        if [ ! -d "$rootdir/users" ]; then
            echo "Error cannot find the mount point of user registry"
            exit 1
        fi
    fi
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
    python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$_gidval}" ./groups/$_group/gid #> /dev/null
    echo $_gidval > ./groups/$_group/gid
    touch ./gids/$_gidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$_group\"}" ./gids/$_gidval #> /dev/null
    echo $_group > ./gids/$_gidval
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
    for i in $_gdir; do
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
    for i in $_udir; do
        _uname=`cat $i`
        echo $_uname $i
    done
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
    rm $_udir/disabled
}
