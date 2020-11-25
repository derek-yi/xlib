
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <fcntl.h>

#include "dev_msg.h"
#include "dev_cmd.h"

int dev_cmd_init()
{
    cli_cmd_reg("list",         "list device",              &cli_dev_list);
    cli_cmd_reg("connect",      "connect device",           &cli_dev_connect);
    cli_cmd_reg("echo",         "send echo msg",            &cli_send_echo);
    
    return 0;
}

int main(int argc, char **argv)
{
    int listen_port = -1;
    int flags = 0;
    int ret;
    
    if (argc < 2) {
        printf("usage: %s <appid> [<listen-port>]\n", argv[0]);
        return 0;
    }

    if (argc > 2) {
        listen_port = atoi(argv[2]);
    }

    ret = dev_msg_init(atoi(argv[1]), listen_port);
    if (ret < 0) {
        printf("dev_msg_init failed, ret %d\n", ret);
        return 0;
    }

    cli_cmd_init();
    dev_cmd_init();
    cli_main_task();
    
    return 0;
}

/*
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH$:../

*/
