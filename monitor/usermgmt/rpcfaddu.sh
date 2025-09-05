#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh
  
OPTIONS="afk:o:g:hp"
   

function Usage()
{
cat << EOF
Usage: $0 [-hpf][-k <kerberos user> ] [ -o <OAuth2 user> ] [ -g group ] <user name> [<user id>]
    <user name> the user name to add, which must be a valid user name.
    <user id> is a integer number that associates with the user name.
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
    k) krb5user=${OPTARG}
        ;;
    o) oa2user=${OPTARG}
        ;;
    p) askPass=1
        ;;
    g) group=${OPTARG}
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

if [[  "x$@" == "x" ]]; then
   echo "Error user name is not specified"
   Usage
   exit 1
fi

uname="$1"
uidval=

source $pubfuncs
check_user_mount

pushd $rootdir > /dev/null
if [[ ! -z "$2" ]]; then
    if [[ "$2" =~ ^-?[0-9]+$ ]] && (( $2 > 0 )) && (( $2 < 1000000 )); then
        uidval="$2"
    else
        echo Error invalid user id
        Usage
        exit 1
    fi

    if [ -f ./uids/$uidval ]; then
        echo "Error user id '$uidval' already exists"
        exit 1
    fi
fi

# check if group is valid if specified
if [ "x$group" != "x" ]; then
    if [ ! -d ./groups/$group ]; then
        echo "Error the group specified does not exist."
        exit 1
    fi
else
    group="default"
fi

echo start adding user $uname ...
if [ -d ./users/$uname ] && [ "$force" == "false" ]; then
    echo "Error user '$uname' already exists"
    exit 1
fi
mkdir -p ./users/$uname || [ "$force" == "true" ]
udir=$rootdir/users/$uname
if [ ! -f $udir/uid ];then
    touch $udir/uid
fi

if [[ -z "$uidval" ]]; then
    uidval=`python3 ${updattr} -a 'user.regfs' 1 ./uidcount`
fi

python3 $updattr -u 'user.regfs' "{\"t\":3,\"v\":$uidval}" $udir/uid > /dev/null
echo $uidval > $udir/uid

if (( $askPass == 1 )); then
    set_password $uname
fi

if [ ! -d $udir/groups ]; then
    mkdir $udir/groups
fi

# link the user to the group's users dir
# and link the group to the user's groups dir
join_group $group $uname

if [ "x$krb5user" != "x" ]; then
    assoc_krb5user $krb5user $uname
fi

if [ "x$oa2user" != "x" ]; then
    assoc_oa2user $oa2user $uname
fi
datestr=`date` 
touch $udir/date
python3 $updattr -u 'user.regfs' "{\"t\":7,\"v\":\"$datestr\"}" $udir/date > /dev/null 
echo $datestr > $udir/date

popd > /dev/null
if (( $mt == 2 )); then
    if [ -d $rootdir ]; then
        umount $rootdir
        rmdir $rootdir > /dev/null 2>&1
    fi
fi
echo "added user $uname done"
