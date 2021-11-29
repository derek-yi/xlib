#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "xmodule.h"

#define MSG_TYPE_TIMER_MSG      (XMSG_T_USER_START + 1)
#define MSG_TYPE_USER_CFG       (XMSG_T_USER_START + 2)

int timer_msg_proc(DEVM_MSG_S *rx_msg)
{
    if (!rx_msg) return VOS_ERR;
	
    printf("%s: %s \n", __FUNCTION__, rx_msg->msg_payload);

    return VOS_OK;
}

void* demo_main_task(void *param)  
{
    devm_set_msg_func(MSG_TYPE_TIMER_MSG, timer_msg_proc);

    while (1) {
        //todo

        vos_msleep(100);
    }
    
    return NULL;
}

int demo_timer_func(void *param)
{
    char usr_msg[128];
    static int timer_cnt = 100;

	//send msg to self
    snprintf(usr_msg, sizeof(usr_msg), "%s %d", get_app_name(), timer_cnt++);
    app_send_msg(0, get_app_name(), MSG_TYPE_TIMER_MSG, usr_msg, strlen(usr_msg) + 1);
    
    return VOS_OK;
}

TIMER_INFO_S my_timer_list[] = 
{
    {1, 30, 0, demo_timer_func, NULL}, 
};

int demo_timer_callback(void *param)
{
    static uint32 timer_cnt = 0;
    
    if (sys_conf_geti("demo_timer_disable")) {
        return VOS_OK;
    }
    
    timer_cnt++;
    for (int i = 0; i < sizeof(my_timer_list)/sizeof(TIMER_INFO_S); i++) {
        if ( (my_timer_list[i].enable) && (timer_cnt%my_timer_list[i].interval == 0) ) {
            my_timer_list[i].run_cnt++;
            if (my_timer_list[i].cb_func) {
                my_timer_list[i].cb_func(my_timer_list[i].cookie);
            }
        }
    }
    
    return VOS_OK;
}

int main(int argc, char **argv)
{
    int ret;
    char cfg_file[128];
    pthread_t threadid;
    timer_t timer_id;

    if (argc < 2) {
		printf("usage: %s <app_name> <cfg_file> \r\n", argv[0]);
		return VOS_ERR;  
    }

    if (argc > 2) {
		snprintf(cfg_file, sizeof(cfg_file), "%s", argv[2]);
    } else {
		snprintf(cfg_file, sizeof(cfg_file), "%s_cfg.txt", argv[1]);
	}
	
    xmodule_init(argv[1], cfg_file);

    ret = pthread_create(&threadid, NULL, demo_main_task, NULL);  
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "pthread_create failed(%s)", strerror(errno));
        return VOS_ERR;  
    } 

    ret = vos_create_timer(&timer_id, 1000, 1, demo_timer_callback, NULL);
    if (ret != 0)  {  
        xlog(XLOG_ERROR, "vos_create_timer failed");
        return -1;  
    } 
    
    pthread_join(threadid, NULL);
}



