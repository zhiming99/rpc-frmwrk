#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh
  
OPTIONS="hik:o:g:p"

function Usage()
{
cat << EOF
Usage: $0 [-hik:o:g:p] <user name> 
    this command modify the attributes of user
    -k <krb5 user name> assocate the kerberos user name to <user name>
    -o <OAuth2 user name> assocate the OAuth2 user name to <user name>
    -g <group name> add <user name> to the group <group name>
    -p set the password for <user name>
    -i inverse the operation on options '-k' '-o' '-g' '-p'. That is
        '-ik" to unassociate the kerberos name with <user name>
        '-io" to unassociate the OAuth2 name with <user name>
        '-ig" to delete <user name> from the group.
        '-ip' to clear the password for <user name>
    -h: print this help
EOF
}
askPass=0     
inverse=0
while getopts $OPTIONS opt; do
    case "$opt" in
    k)
        krb5user=$2
        ;;
    o)
        oa2user=$2
        ;;
    p)
        askPass=1
        ;;
    g)
        group=$2
        ;;
    i)  inverse=1
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

source $pubfuncs
check_user_mount

pushd $rootdir

echo start modifying user\(s\) $@ ...

for uname in "$@"; do
    if [ "x$krb5user" != "x" ]; then
        if (( $inverse == 0 )); then
            assoc_krbuser $krb5user $uname
        else
            unassoc_krbuser $krb5user $uname
        fi
    fi
    if [ "x$oa2user" != "x" ]; then
        if (( $inverse == 0 )); then
            assoc_krbuser $oa2user $uname
        else
            unassoc_krbuser $oa2user $uname
        fi
    fi
    if [ "x$group" != "x" ]; then
        if (( $inverse == 0 )); then
            join_group $group $uname
        else
            leave_group $group $uname
        fi
    fi
    if (( $askPass == 1 ));then
        if (( $inverse == 0 )); then
            set_password $uname
        else
            clear_password $uname
        fi
    fi
done
popd


