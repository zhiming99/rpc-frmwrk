#!/bin/bash
 
script_dir=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
updattr=${script_dir}/updattr.py
appfuncs=${script_dir}/appfuncs.sh
rpcfshow=${script_dir}/rpcfshow.sh
  
OPTIONS="su:g:h"

function Usage()
{
cat << EOF
Usage: $0 [-h] [-u <user name> ][-g <group name> ]
    This command initialize the application registry.
    -h: Print this help
    -u: Set the owner of the application instances. Default is 'admin'
    -g: Set the group of the application instances. Default is 'admin'.
    -s: format the app registry silently
EOF
}
     
bSilent=0
while getopts $OPTIONS opt; do
    case "$opt" in
    u)  username=${OPTARG}
        ;;
    g)  groupname=${OPTARG}
        ;;
    s)  bSilent=1
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
                                                                                                  
if ! which regfsmnt > /dev/null; then
    echo cannot find program 'regfsmnt'. You may want \
        to install rpc-frmwrk first
    exit 1
fi

if [ -z $username ]; then
    username='admin'
fi
if [ -z $groupname ]; then
    groupname='admin'
fi

#echo haha
#echo "bash $rpcfshow -u $username | grep 'uid:' | awk '{print $2}'"
uid=`bash $rpcfshow -u $username | grep 'uid:' | awk '{print $2}'`
gid=`bash $rpcfshow -g $groupname | grep 'gid:' | awk '{print $2}'`
if [ -z $uid ] || [ -z $gid ]; then
    echo "Error the given user/group not valid"
    exit 22
fi

pushd ${HOME} > /dev/null
if [ ! -d .rpcf ]; then mkdir .rpcf; fi
cd .rpcf
if [ ! -d ./testmnt ]; then mkdir ./testmnt; fi
if [ -f appreg.dat ]; then
   if (( $bSilent==0 ));then
       echo "you are going to format the application registry, continue ( y/n )?(default: yes)"
       read answer
       if [ "x$answer" == "xy" ] || [ "x$answer" == "xyes" ] || [ "x$answer" == "x" ]; then
           mv appreg.dat appreg.dat.bak
           regfsmnt -i ./appreg.dat || exit $?
       else
           exit 0
       fi
    else
       mv appreg.dat appreg.dat.bak
       regfsmnt -i ./appreg.dat || exit $?
    fi
else
    regfsmnt -i ./appreg.dat || exit $?
fi

regfsmnt -d ./appreg.dat ./testmnt

source $appfuncs
if ! wait_mount ./testmnt 10;then
    popd
    exit $?
fi

approot=./testmnt
pushd $approot > /dev/null

mkdir -p apps

echo adding application timer1
add_stdapp timer1
set_point_value timer1 cmdline "$(jsonval 'blob' ${script_dir}/apptimer' -gd')" blob
set_point_value timer1 working_dir  "$(jsonval 'blob' '/' )" blob
add_point timer1 clock1 output i
set_attr_value timer1 clock1 pulse "$(jsonval 'i' 1 )" i

add_point timer1 interval1 setpoint i
set_attr_value timer1 interval1 unit "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval1 "$(jsonval 'i' 10)" i
set_attr_value timer1 interval1 load_on_start "$(jsonval 'i' 1)" i

add_point timer1 clock2 output i
set_attr_value timer1 clock2 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval2 setpoint i
set_attr_value timer1 interval2 unit "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval2 "$(jsonval 'i' 2 )" i
set_attr_value timer1 interval2 load_on_start "$(jsonval 'i' 1)" i

add_point timer1 clock3 output i
set_attr_value timer1 clock3 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval3 setpoint i
set_attr_value timer1 interval3 unit "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval3 "$(jsonval 'i' 20)" i

add_point timer1 clock4 output i
set_attr_value timer1 clock4 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval4 setpoint i
set_attr_value timer1 interval4 unit "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval4 "$(jsonval 'i' 40)" i

add_point timer1 offline_action input s
set_attr_value timer1 offline_action point_flags "$(jsonval 'i' 1 )" i

add_point timer1 schedule1 setpoint blob
schedstr='0 1 9 * * ?'
valstr=$(jsonval 'blob' "$schedstr")
tmpfile=$(mktemp -u)
echo "$valstr" > $tmpfile
#to prevent globing of aesterisk
set_point_value timer1 schedule1 "$(cat $tmpfile)" blob
rm $tmpfile
add_point timer1 sched_task1 output i
set_attr_value timer1 sched_task1 pulse "$(jsonval 'i' 1 )" i
set_attr_value timer1 sched_task1 lastrun "$(jsonval 'i' 0)" i
set_attr_value timer1 sched_task1 nextrun "$(jsonval 'i' 0)" i
set_attr_value timer1 schedule1 load_on_start "$(jsonval 'i' 1)" i
set_point_value timer1 app_class "$(jsonval 's' 'timer')" s

echo adding application appmonsvr1
add_stdapp appmonsvr1
set_point_value appmonsvr1 cmdline "$(jsonval 'blob' 'appmonsvr -gd '$HOME/.rpcf/appmonroot)" blob
set_point_value appmonsvr1 working_dir  "$(jsonval 'blob' '/' )" blob

# point logger
add_point appmonsvr1 ptlogger1 input i
add_link timer1 clock2 appmonsvr1 ptlogger1
set_attr_value appmonsvr1 ptlogger1 ptlist "$(jsonval 'blob' '[]' )" blob
add_point appmonsvr1 logrotate1 input i
add_link timer1 sched_task1 appmonsvr1 logrotate1

echo adding application loggersvr1
add_stdapp loggersvr1
set_point_value loggersvr1 cmdline "$(jsonval 'blob' 'rpcf_logger -od')" blob
set_point_value loggersvr1 working_dir  "$(jsonval 'blob' '/' )" blob
add_point loggersvr1 logcontent output blob
add_point loggersvr1 lines  setpoint i
set_point_value loggersvr1 lines "$(jsonval 'i' 100 )" i
set_attr_value loggersvr1 lines load_on_start "$(jsonval 'i' 1)" i
set_point_value loggersvr1 app_class "$(jsonval 's' 'logger')" s

change_application_owner timer1 $uid $gid
change_application_owner appmonsvr1 $uid $gid
change_application_owner loggersvr1 $uid $gid

add_rpcrouter rpcrouter1
chown $uid:$gid ./apps 

#leaving approot
popd > /dev/null
fusermount3 -u ./testmnt
rmdir ./testmnt
if [ ! -d appmonroot ]; then
    mkdir appmonroot
fi
popd > /dev/null
