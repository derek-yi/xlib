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

long vos_get_ts(void)
{
    return (long)time(NULL);
}

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