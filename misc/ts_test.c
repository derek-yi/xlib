#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/ioctl.h>

int vos_run_cmd(char* cmd_str)
{
    int status;

    if (NULL == cmd_str)
    {
        return 1;
    }

    status = system(cmd_str);
    if (status < 0)
    {
        // log_info("cmd: %s, error: %s", cmd_str, strerror(errno));
        return status;
    }

    if (WIFEXITED(status))
    {
        // printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
        return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        // printf("abnormal termination,signal number =%d\n", WTERMSIG(status)); //如果cmdstring被信号中断，取得信号值
        return 1;
    }
    else if (WIFSTOPPED(status))
    {
        // printf("process stopped, signal number =%d\n", WSTOPSIG(status)); //如果cmdstring被信号暂停执行，取得信号值
        return 1;
    }

    return 0;
}

long vos_get_ts(void)
{
    return (long)time(NULL);
}

#if 0
int main()
{
    int ret;
    char *time_str = "2024-09-04 10:30:33";
    struct tm tmd;
    long time_diff;

    ret = sscanf(time_str, "%d-%d-%d %d:%d:%d", 
                 &tmd.tm_year, &tmd.tm_mon, &tmd.tm_mday, &tmd.tm_hour, &tmd.tm_min, &tmd.tm_sec);
    if (ret < 6 || tmd.tm_year < 1900 || tmd.tm_mon < 1) {
        printf("illegal time \n");
        return 0;
    } else {
        tmd.tm_year = tmd.tm_year - 1900;
        tmd.tm_mon = tmd.tm_mon - 1;
        tmd.tm_isdst = -1;
        time_diff = (long)mktime(&tmd) - vos_get_ts();
        printf("WARN: %ld - %ld = %ld \n", mktime(&tmd), vos_get_ts(), time_diff);
    }

    return 0;
}

#endif

int main()
{
    int ret;
    struct tm *tp;
    struct tm this_tm;
    time_t t, t2; //long

    t = time(&t2);
    tp = localtime(&t);
	printf("t %lu, t2 %lu\n", t, t2);

    localtime_r(&t, &this_tm);
    printf("this_tm: %d-%d-%d %d:%d:%d\n", 
           this_tm.tm_year + 1900, this_tm.tm_mon + 1, this_tm.tm_mday, this_tm.tm_hour, this_tm.tm_min, this_tm.tm_sec);
	
    t = t + 10;
    //gmtime_r(&t, &this_tm);
    localtime_r(&t, &this_tm);

		
    printf("tp: %d-%d-%d %d:%d:%d\n", 
           tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);

    printf("this_tm: %d-%d-%d %d:%d:%d\n", 
           this_tm.tm_year + 1900, this_tm.tm_mon + 1, this_tm.tm_mday, this_tm.tm_hour, this_tm.tm_min, this_tm.tm_sec);
		   
	//ret = vos_run_cmd("mosquitto_pub -h 192.168.100.26  -t  \"testtopic/2\" -i \"client3\" -m \"How are you?\" -u client3 -P xp123456");
	printf("xxx = %d \n", sizeof(""));
 
    return 0;
}