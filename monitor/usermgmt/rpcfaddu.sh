#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
  
OPTIONS="afk:o:g:hp"
   

function Usage()
{
cat << EOF
Usage: $0 [-hpf][-k <kerberos user> ] [ -o <OAuth2 user> ] [ -g group ] <user name>
    <user name> the user name to add, which must be a valid Linux user name.
    -h: print this help
    -f: force adding a user whether the user exists or not
    -p: ask for password
    -k <kerberos user> : associate a kerberos user name to <user name>, so that
    when login via kerberos, the kerberos user is mapped to <user name>, as will
    be used to access app monitor.
    -o <OAuth2 user> : associate a OAuth2 user name to <user name>, so that
    when login via OAuth2, the OAuth2 user is mapped to <user name>, as will be
    used to access app moniter.
    -g <group name>: group this user belogs to. The group must be a valid group.
    if not specified, the user will go to group 'default'. And the user can join
    other groups by command 'rpcfmodu'
EOF
}
     
force="false"
askPass=0
while getopts $OPTIONS opt; do
    case "$opt" in
    k) krb5user=$2
        ;;
    o) oa2user=$2
        ;;
    p) askPass=1
        ;;
    g) group=$2
        ;;
    f) force="true"
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

if [  "x$@" == "x" ]; then
   echo "Error user name is not specified"
   Usage
   exit 1
fi
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

# check if group is valid if specified
if [ "x$group" != "x" ]; then
    if [ ! -d $rootdir/groups/$group ]; then
        echo "Error the group specified does not exist."
        exit 1
    fi
else
    group="default"
fi

pushd $rootdir
ls -R .
echo start adding user $@ ...
for uname in "$@"; do
    if [ -d ./users/$uname ] && [ "$force" == "false" ]; then
        echo "Error user '$uname' already exists"
        exit 1
    fi
    mkdir -p ./users/$uname || [ "$force" == "true" ]
    udir=$rootdir/users/$uname
    if [ ! -f $udir/uid ];then
        touch $udir/uid
        uidval=`python3 ${updattr} -a 'user.regfs' 1 ./uidcount`
        python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$uidval}" $udir/uid > /dev/null
        echo $uidval > $udir/uid
    else
        uidval=`python3 ${updattr} -v $udir/uid`
        if [ "$uidval" == "None" ]; then
            #uid was not set due to last failure
            uidval=`python3 ${updattr} -a 'user.regfs' 1 ./uidcount`
            python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$uidval}" $udir/uid > /dev/null
            echo $uidval > $udir/uid
        fi
    fi
    if (( $askPass == 1 )); then
        touch $udir/passwd
        echo -n Password:
        read -s password
        echo
        echo -n Enter again:
        read -s password1
        if [ "$password1" != "$password" ]; then
            echo Error the password was not enterred correctly
            echo you can change the password later with rpcfmodu
            exit 1
        fi
        passhash=`echo -n $password | sha1sum | awk '{print $1}'`
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$passhash\"}" $udir/passwd > /dev/null 
    fi

    if [ ! -d $udir/groups ]; then
        mkdir $udir/groups
    fi

    # link the user to the group's users dir
    # and link the group to the user's groups dir
    gidval=`python3 $updattr -v ./groups/$group/gid`

    echo setup links to groups
    touch $udir/groups/$gidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$group\"}" $udir/groups/$gidval > /dev/null
    echo $group > $udir/groups/$gidval

    touch $rootdir/groups/$group/users/$uidval
    file=$rootdir/groups/$group/users/$uidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$uname\"}" $file > /dev/null
    echo $uname > $file

    touch $rootdir/uids/$uidval
    file=$rootdir/uids/$uidval
    python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$uname\"}" $file > /dev/null
    echo $uname > $file

    if [ "x$krb5user" != "x" ]; then
        if [ ! -d $udir/krb5users ];then
            mkdir $udir/krb5users
        fi
        touch $udir/krb5users/$krb5users
        if [ ! -f ./krb5users/$krb5user ]; then
            touch ./krb5users/$krb5user
        fi
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$uname\"}" ./krb5users/$krb5user > /dev/null
        echo $uname > ./krb5users/$krb5user
    fi

    if [ "x$oa2user" != "x" ]; then
        if [ ! -d $udir/oa2users ];then
            mkdir $udir/oa2users
        fi
        touch $udir/oa2users/$oa2user
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$uname\"}" ./oa2users/$oa2user > /dev/null
        echo $uname > ./krb5users/$oa2user
    fi

done
popd
if (( $mt == 2 )); then
    if [ -d $rootdir ]; then
        umount $rootdir
        rmdir $rootdir
    fi
fi
