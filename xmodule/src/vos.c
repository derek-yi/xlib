
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>   //timer_t
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "vos.h"

#ifndef MAKE_XLIB

#define vos_print   printf

#endif


#define LINE_BUF_SZ		256

/* each line: key_str=val_buf */
int cfgfile_read_str(char *file_name, char *key_str, char *val_buf, int buf_len)
{
	FILE *fp;
	char *ptr;
	char line_str[LINE_BUF_SZ];
	int cp_len;

    if (file_name == NULL) return -1;
    if (key_str == NULL || val_buf == NULL) return -1;
    
	fp = fopen(file_name, "r");
    if (fp == NULL) {
        return -2;
    }
    
    while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
		line_str[strlen(line_str)] = 0; //delete '\n'
		
		if (memcmp(line_str, key_str, strlen(key_str)) == 0) { //key_str must at line head
			ptr = line_str + strlen(key_str);
			while ( ptr != NULL) {
				if (*ptr == '=' || *ptr == ' ') ptr++;
				else break;
			}
			
			if (ptr != NULL) {
				cp_len = strlen(ptr);
				if (cp_len > buf_len - 1) cp_len = buf_len - 1;
				memcpy(val_buf, ptr, cp_len);
				val_buf[cp_len] = 0;
				fclose(fp);
				return 0;
			}
		}
    }

	fclose(fp);
    return -3;
}

int cfgfile_write_str(char *file_name, char *key_str, char *val_str)
{
	FILE *fp;
	struct stat fs;
	char line_str[LINE_BUF_SZ];
	char *file_buff;
	int find_node = 0;
	int buf_ptr = 0;

	if (file_name == NULL) return -1;
	if (key_str == NULL || val_str == NULL) return -1;

	if (stat(file_name, &fs) == -1) {
       fs.st_size = 0;
    }

	file_buff = (char *)malloc(fs.st_size + LINE_BUF_SZ);
	if (file_buff == NULL) {
		//perror("malloc");
		return -2;
	}
	memset(file_buff, 0, fs.st_size + LINE_BUF_SZ);

	fp = fopen(file_name, "r");
	if (fp != NULL) {
		while (fgets(line_str, LINE_BUF_SZ, fp) != NULL) { 
			//printf("readline: %s \n", line_str);
			if (line_str[0] == 0) break;
			
			if (memcmp(line_str, key_str, strlen(key_str)) == 0) { //key_str must at line head
				find_node = 1;
				if (val_str == NULL) { //clear
					//printf("clear %s\n", key_str);
					continue; 
				} else {  //update
					snprintf(line_str, LINE_BUF_SZ, "%s=%s\n", key_str, val_str);
					//printf("update %s\n", key_str);
				}
			} 
			memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
			buf_ptr += strlen(line_str);
		}

		fclose(fp);
	}

	if (find_node == 0) {
		//printf("add %s\n", key_str);
		snprintf(line_str, LINE_BUF_SZ, "%s=%s\n", key_str, val_str);
		memcpy(file_buff + buf_ptr, line_str, strlen(line_str));
		buf_ptr += strlen(line_str);
	}

	fp = fopen(file_name, "w+");
	if (fp == NULL) {
		//perror("fopen");
		return -2;
	}
	fwrite(file_buff, buf_ptr, 1, fp);
	fclose(fp);
	
	return 0;
}

int pipe_read(char *cmd_str, char *buff, int buf_len)
{
	FILE *fp;

    if (!cmd_str) return VOS_ERR;
    if (!buff) return VOS_ERR;
    
	fp = popen(cmd_str, "r");
    if (fp == NULL) {
        vos_print("popen failed(%s), cmd(%s)\n", strerror(errno), cmd_str);
        return VOS_ERR;
    }
    
    memset(buff, 0, buf_len);
	fgets(buff, buf_len, fp);
	pclose(fp);
    return VOS_OK;
}

int sys_node_readstr(char *node_str, char *rd_buf, int buf_len)
{
	FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;
    if (rd_buf == NULL) return VOS_ERR;
    
    snprintf(cmd_buf, sizeof(cmd_buf), "cat %s", node_str);
	fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        vos_print("popen failed(%s), cmd(%s)\n", strerror(errno), cmd_buf);
        return VOS_ERR;
    }
    
    memset(rd_buf, 0, buf_len);
	fgets(rd_buf, buf_len, fp);
	pclose(fp);

    return VOS_OK;
}

int sys_node_read(char *node_str, int *value)
{
	FILE *fp;
    char cmd_buf[256];
    char rd_buf[32];

    if (node_str == NULL) return VOS_ERR;
    
    snprintf(cmd_buf, sizeof(cmd_buf), "cat %s", node_str);
	fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        vos_print("popen failed(%s), cmd(%s)\n", strerror(errno), cmd_buf);
        return VOS_ERR;
    }
    
	fgets(rd_buf, 30, fp);
	pclose(fp);

    if (value) {
        *value = (int)strtoul(rd_buf, 0, 0);
    }
    
    return VOS_OK;
}

int sys_node_writestr(char *node_str, char *wr_buf)
{
    FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;

    snprintf(cmd_buf, sizeof(cmd_buf), "echo %s > %s", wr_buf, node_str);
    fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        vos_print("popen failed(%s), cmd(%s)\n", strerror(errno), cmd_buf);
        return VOS_ERR;
    }
    pclose(fp);
    
    return VOS_OK;
}

int sys_node_write(char *node_str, int value)
{
    FILE *fp;
    char cmd_buf[256];

    if (node_str == NULL) return VOS_ERR;

    snprintf(cmd_buf, sizeof(cmd_buf), "echo 0x%x > %s", value, node_str);
    fp = popen(cmd_buf, "r");
    if (fp == NULL) {
        vos_print("popen failed(%s), cmd(%s)\n", strerror(errno), cmd_buf);
        return VOS_ERR;
    }
    pclose(fp);
    
    return VOS_OK;
}

/*
union sigval {
    int sival_int;
    void *sival_ptr;
};
*/
typedef void (* lib_callback)(union sigval);

int vos_create_timer(timer_t *ret_tid, int interval, timer_cb callback, void *param)
{
	timer_t timerid;
	struct sigevent evp;

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_value.sival_ptr = param; 
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = (lib_callback)callback;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
        vos_print("timer_create failed(%s)\n", strerror(errno));
		return 1;
	}
	
	//第一次间隔it.it_value这么长,以后每次都是it.it_interval这么长
	struct itimerspec it;
	it.it_interval.tv_sec = interval;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = interval;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, NULL) == -1) {
        vos_print("timer_settime failed(%s)\n", strerror(errno));
        timer_delete(timerid);
		return 1;
	}

    if (ret_tid) {
        *ret_tid = timerid;
    }

    return 0;
}

void vos_msleep(uint32 milliseconds) 
{
    struct timespec ts = {
        milliseconds / 1000,
        (milliseconds % 1000) * 1000000
    };
    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno));
}

//https://blog.csdn.net/Primeprime/article/details/60954203
int vos_run_cmd(char *cmd_str)
{
    int status;
    
    if (NULL == cmd_str)
    {
        return VOS_ERR;
    }
    
    status = system(cmd_str);
    if (status < 0)
    {
        vos_print("cmd: %s, error: %s", cmd_str, strerror(errno));
        return status;
    }
     
    if (WIFEXITED(status))
    {
        //printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
        return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        //printf("abnormal termination,signal number =%d\n", WTERMSIG(status)); //如果cmdstring被信号中断，取得信号值
        return VOS_ERR;
    }
    else if (WIFSTOPPED(status))
    {
        //printf("process stopped, signal number =%d\n", WSTOPSIG(status)); //如果cmdstring被信号暂停执行，取得信号值
        return VOS_ERR;
    }

    return VOS_OK;
}

int vos_sem_wait(void *sem_id, uint32 msecs)
{
	struct timespec ts;
    sem_t *sem = (sem_t *)sem_id;
	uint32 add = 0;
	uint32 secs = msecs/1000;
    
	clock_gettime(CLOCK_REALTIME, &ts);

	msecs = msecs%1000;
	msecs = msecs*1000*1000 + ts.tv_nsec;
	add = msecs / (1000*1000*1000);
	ts.tv_sec += (add + secs);
	ts.tv_nsec = msecs%(1000*1000*1000);
 
	return sem_timedwait(sem, &ts);
}


