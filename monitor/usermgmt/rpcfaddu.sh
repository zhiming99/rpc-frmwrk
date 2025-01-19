#!/bin/bash
#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
  
OPTIONS="afk:o:g:h"
   

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
while getopts $OPTIONS opt; do
    case "$opt" in
    -k) krb5user=$2
        ;;
    -o) oa2user=$2
        ;;
    -p) askPass=1
        ;;
    -g) group=$2
        ;;
    -f) force="true"
        ;;
    -h)
        Usage
        exit 0
        ;;
    \?)
        echo "Invalid option: -$OPTARG >&2"
        exit 1
        ;;
    esac done
                                                                                                  
shift $((OPTIND-1))

if [  "$@" == "" ]; then
   echo "Error user name is not specified"
   Usage
fi
mp=`mount | grep regfsmnt`
if [ "$mp" == "" ];then
    appmp=`mount | grep appmnt`
fi
mt=0
if [ "$mp" == "" ] && [ "$appmp" == "" ]; then
    mt=2
elif [ "$appmp" != "" ]; then
    mt=1
else
    mt=0
fi
base=$HOME/.rpcf
if [ -f $base/usereg.dat ]; then
    echo "Error, did not find the user registry file."
    echo "you may want to use 'inituser.sh' to initialize one first"
fi
if (( $mt == 2 ));then
    rootdir="$base/testmnt"
    mkdir -p $rootdir
    if ! regfsmnt -d $base/usereg.dat $rootdir; then
        echo "Error, failed to mount usereg.dat"
        exit 1
    fi
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
if [ "$group" != "" ]; then
    if [ ! -d $rootdir/groups/$group ]; then
        echo "Error the group specified does not exist."
        exit 1
    fi
else
    group = "default"
fi

pushd $rootdir
for uname in "$@"; do
    if [ -d ./users/$uname ] && [ "$force" == "true" ]; then
        echo "Error the user already exists"
        exit 1
    fi
    mkdir -p ./users/$uname || [ "$force" == "true" ]
    udir=./users/$uname
    if [ ! -f $udir/uid ];then
        touch $udir/uid
        uidval=`python3 ${updattr} -a 'user.regfs' 1 ./uidcount`
        python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$uidval}" $udir/uid
    else
        uidval=`python3 ${updattr} -v $udir/uid`
        if [ "$uidval" == "None" ]; then
            #uid was not set due to last failure
            uidval=`python3 ${updattr} -a 'user.regfs' 1 ./uidcount`
            python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$uidval}" $udir/uid
        fi
    fi
    mkdir $udir/groups
    # link the user to the group's users dir
    # and link the group to the user's groups dir
    gidval=`python3 $updattr -v $roodir/groups/$group/gid`
    ln -s ./groups/$group $udir/groups/$gidval
    ln -s $udir ./groups/$group/users/$uidval
    ln -s $udir ./uids/$uidval

    if [ "$krb5user" != "" ]; then
        touch $udir/krb5user
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$krb5user\"}" $udir/krb5user
        if [ ! -d ./krb5users/$krb5user ]; then
            mkdir ./krb5users/$krb5user
        fi
        ln -s $udir ./krb5users/$krb5user/$uidval
    fi

    if [ "$oa2user" != "" ]; then
        touch $udir/oa2user
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$oa2user\"}" $udir/oa2user
        if [ ! -d ./oa2users/$oa2user ]; then
            mkdir ./oa2users/$oa2user
        fi
        ln -s $udir ./oa2users/$oa2user/$uidval
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
        passhash= `echo -n $password | sha1sum | awk '{print $1}'`
        python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$passhash\"}" $udir/passwd
    fi
done
popd
