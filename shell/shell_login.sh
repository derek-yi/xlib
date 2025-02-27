#!/bin/bash

SCRIPT_NAME=`basename $0`
CURRENT_DIR=$(cd "$(dirname "$0")";pwd)

execute_ssh_cmd()
{
    local host_ip=$1
    local user_name=$2
    local user_password=$3
    local cmd="$4"
    local log_file=${CURRENT_DIR}/execute_ssh_cmd.log
    
    # 如果密码中包含$符号，需要转义以下
    user_password=`echo ${user_password} | sed 's/\\$/\\\\$/g'`
    
    /usr/bin/expect <<EOF > ${log_file}
    set timeout -1
    spawn ssh ${user_name}@${host_ip}
    expect {
        "(yes/no)?"
        {
            send "yes\n"
            expect "*assword:" { send "${user_password}\n"}
        }
        "*assword:"
        {
            send "${user_password}\n"
        }
    }
    sleep 1
    send "${cmd}\n"
    send "exit\n"
    expect eof
EOF

   cat ${log_file} | grep -iE "Permission denied|failed" >/dev/null
   if [ $? -eq 0 ];then
        echo "Script execute failed!"
        return 1
   fi
   return 0
}

# $0 127.0.0.1 derek apple "cd /home/derek/share/misc; ./aa.sh"
execute_ssh_cmd "$@"