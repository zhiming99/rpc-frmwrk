#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
pubfuncs=${script_dir}/pubfuncs.sh
  
OPTIONS="hugko"

function Usage()
{
cat << EOF
Usage: $0 [-hugko] <user name|group name> 
    this command show the infomation about the object
    -u: show user information
    -g: show group information
    -k: show krb5 associations
    -o: show OAuth2 associations
    -h: print this help
EOF
}
     
while getopts $OPTIONS opt; do
    case "$opt" in
    g)  show="group"
        ;;
    u)  show="user"
        ;;
    k)  show="krb5"
        ;;
    o)  show="OAuth2"
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

if [ "x$show" == "x" ]; then
   echo "Error missing '-ugko' option"
   Usage
   exit 1
fi

if [  "x$@" == "x" ] && ( [ "x$show" == "xuser" ] || [ "x$show" == "xgroup" ] ); then
   echo "Error user/group name is not specified"
   Usage
   exit 1
fi


source $pubfuncs
check_user_mount

pushd $rootdir > /dev/null
if [ "x$show" == "xuser" ]; then
    for uname in "$@"; do
        show_user $uname
    done
elif [ "x$show" == "xgroup" ]; then
    for group in "$@"; do
        show_group $group
    done
elif [ "x$show" == "xkrb5" ]; then
    show_krb5_assoc
elif [ "x$show" == "xOAuth2" ]; then
    show_oauth2_assoc
fi
popd > /dev/null

