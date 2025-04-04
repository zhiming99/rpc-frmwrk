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
    exit $?
fi

approot=./testmnt
pushd $approot > /dev/null

mkdir -p apps

echo adding application rpcrouter1
add_application rpcrouter1

add_point rpcrouter1 rpt_timer input i
set_attr_value rpcrouter1 rpt_timer unit "$(jsonval 'i' 0 )" i

#point contents are in json
add_point rpcrouter1 sessions output blob
add_point rpcrouter1 bdge_list output blob
add_point rpcrouter1 bdge_proxy_list output blob
add_point rpcrouter1 req_proxy_list output blob 

add_point rpcrouter1 max_conn  setpoint i
add_point rpcrouter1 conn_count  output i
add_point rpcrouter1 max_recv_bps  setpoint i
add_point rpcrouter1 max_send_bps  setpoint i
add_point rpcrouter1 obj_count  output i
add_point rpcrouter1 pending_tasks  output i
add_point rpcrouter1 max_pending_tasks setpoint i
add_point rpcrouter1 restart input i
set_attr_value rpcrouter1 restart pulse "$(jsonval 'i' 1 )" i
add_point rpcrouter1 cmdline setpoint blob
set_point_value rpcrouter1 cmdline "$(jsonval 'blob' 'rpcrouter -agor 2')" blob
add_point rpcrouter1 pid output i 
add_point rpcrouter1 vmsize_kb output q
add_point rpcrouter1 cpu_load output f
add_point rpcrouter1 open_files  output i
add_point rpcrouter1 working_dir  setpoint blob

add_point rpcrouter1 uptime output i
set_attr_value rpcrouter1 uptime unit "$(jsonval 's' 'sec' )" s

add_point rpcrouter1 rx_bytes output qword
add_point rpcrouter1 tx_bytes output qword


echo adding application timer1
add_application timer1
add_point timer1 clock1 output i
add_link timer1 clock1 rpcrouter1 rpt_timer
set_attr_value timer1 clock1 pulse "$(jsonval 'i' 1 )" i

add_point timer1 interval1 setpoint i
set_attr_value timer1 interval1 uint "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval1 "$(jsonval 'i' 10)" i

add_point timer1 clock2 output i
set_attr_value timer1 clock2 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval2 setpoint i
set_attr_value timer1 interval2 uint "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval2 "$(jsonval 'i' 10)" i

add_point timer1 clock3 output i
set_attr_value timer1 clock3 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval3 setpoint i
set_attr_value timer1 interval3 uint "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval3 "$(jsonval 'i' 10)" i

add_point timer1 clock4 output i
set_attr_value timer1 clock4 pulse "$(jsonval 'i' 1 )" i
add_point timer1 interval4 setpoint i
set_attr_value timer1 interval4 uint "$(jsonval 's' 'sec' )" s
set_point_value timer1 interval4 "$(jsonval 'i' 10)" i

add_point timer1 restart input i
set_attr_value timer1 restart pulse "$(jsonval 'i' 1 )" i
add_point timer1 cmdline setpoint s 
add_point timer1 pid output i 

# setting access mode and owners
chown $uid:$gid -R ./apps/rpcrouter1 ./apps/timer1
find . -type f -exec chmod ug+rw,o+r '{}' ';'
find . -type d -exec chmod ug+rwx,o+rx '{}' ';'
chmod -R o-rwx ./apps/rpcrouter1/points/restart

#leaving approot
popd > /dev/null
fusermount3 -u ./testmnt
rmdir ./testmnt
popd > /dev/null
