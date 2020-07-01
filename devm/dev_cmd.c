#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "dev_cmd.h"

typedef struct CMD_NODE
{
    struct CMD_NODE *pNext;

    char  *cmd_str;
    char  *help_str;
    FUNC_ENTRY  cmd_func;    
}CMD_NODE;

#define CMD_BUFF_MAX            1024

#define CMD_ERR_NOT_MATCH       0x0001
#define CMD_ERR_AMBIGUOUS       0x0002

#define CHECK_AMBIGUOUS

char    cli_cmd_buff[CMD_BUFF_MAX];
uint32  cli_cmd_ptr = 0;

CMD_NODE  *gst_cmd_list  = NULL;
uint32     cmd_end_flag  = 0;

int vos_print(const char * format,...)
{
    va_list args;
    char buf[CMD_BUFF_MAX];
    int len;

    va_start(args, format);
    len = vsnprintf(buf, CMD_BUFF_MAX-1, format, args);
    va_end(args);

    printf("%s", buf);

    return len;    
}

uint32 cli_param_format(char *param, char **argv, uint32 max_cnt)
{
    char *ptr = param;
    uint32 cnt = 0;
    uint32 flag = 0;

    while(*ptr == 0)
    {
        if(*ptr != ' ' && *ptr != '\t')
            break;
        ptr++;
    }

    if(*ptr == 0) return 0;
    argv[cnt++] = ptr;
    flag = 1;

    while(cnt < max_cnt && *ptr != 0)
    {
        if(flag == 0 && *ptr != '\t' && *ptr != ' ')
        {
            argv[cnt++] = ptr;
            flag = 1;
        }
        else if(flag == 1 && (*ptr == ' ' || *ptr == '\t'))
        {
            flag = 0;
            *ptr = 0;
        }
        ptr++;
    }

    //fix the last param
    if(*ptr != 0)
    {   
        while(*ptr != 0)
        {
            if(*ptr == ' ' || *ptr == '\t')
            {   
                *ptr = 0;
                break;
            }
            ptr++;
        }
    }

    return cnt;
}

int cli_cmd_exec(char *buff)
{
    uint32  cmd_key_len;
    uint32  cmd_len;
    uint32  i;
    int     argc;
    char   *argv[32];
    int     rc;
    CMD_NODE *pNode;

    cmd_len = strlen(buff);
    if(cmd_len < 1)
    {
        return VOS_OK;
    }

    cmd_key_len = 0;
    for(i=0; i<cmd_len; i++)
    {
        if(buff[i] == 0 || buff[i] == ' ' || buff[i] == '\t')
            break;
        cmd_key_len++;
    }

    pNode = gst_cmd_list;
    while (pNode != NULL)
    {
        if(strncmp(pNode->cmd_str, buff, cmd_key_len) == 0)
        {
            break;
        }
        pNode = pNode->pNext;
    }

    if(pNode == NULL)
    {
        rc = system(buff);
        return rc; 
    }

#ifdef CHECK_AMBIGUOUS
    //if not full match, check ambiguous
    if(cmd_key_len < strlen(pNode->cmd_str))
    {
        CMD_NODE *iNode;
        iNode = pNode->pNext;
        while(iNode != NULL)
        {
            if(memcmp(iNode->cmd_str, buff, cmd_key_len) == 0)
            {
                break;
            }
            iNode = iNode->pNext;
        }   
        
        if(iNode != NULL)
        {
            return CMD_ERR_AMBIGUOUS;
        }
    }
#endif    

    // exec
    argc = cli_param_format(buff, argv, 32);
    rc = (pNode->cmd_func)(argc, argv);
    
    return rc;
}

int cli_cmd_reg(char *cmd, char *help, FUNC_ENTRY func)
{
    CMD_NODE *new_node;
    
    new_node = (CMD_NODE *)malloc(sizeof(CMD_NODE));
    if(new_node == NULL)
    {
        printf("VOS_MALLOC failed!\r\n");
        return VOS_ERR;
    }

    memset(new_node, 0, sizeof(CMD_NODE));
    new_node->cmd_func = func;
    new_node->cmd_str = (char *)strdup(cmd);
    new_node->help_str = (char *)strdup(help);

    printf("cli_cmd_reg: %s(%s) \r\n", new_node->cmd_str, new_node->help_str);
    new_node->pNext = gst_cmd_list;
    gst_cmd_list = new_node;

    return VOS_OK;
}

int cli_do_exit(int argc, char **argv)
{
    cmd_end_flag = 1;
    
    return VOS_OK;
}

int cli_do_param_test(int argc, char **argv)
{
    uint32 i;

    vos_print("param format: \r\n");
    for (i=0; i<argc; i++)
    {
        vos_print("%d: %s\r\n", i, argv[i]);
    }
    
    return VOS_OK;
}

int cli_do_show_version(int argc, char **argv)
{
    vos_print("===================================\r\n");
    vos_print("vos version: 1.0\r\n");
    vos_print("===================================\r\n");
    
    return VOS_OK;
}

int cli_do_help(int argc, char **argv)
{
    CMD_NODE *pNode;
    
    pNode = gst_cmd_list;
    while(pNode != NULL)
    {
        vos_print("%-24s -- %-45s \r\n", pNode->cmd_str, pNode->help_str);
        pNode = pNode->pNext;
    }

    return VOS_OK;
}

void cli_cmd_init(void)
{
    gst_cmd_list = NULL;
    cli_cmd_reg("quit",         "exit app",             &cli_do_exit);
    cli_cmd_reg("help",         "cmd help",             &cli_do_help);
    cli_cmd_reg("version",      "show vos version",     &cli_do_show_version);
    cli_cmd_reg("cmdtest",      "cmd param test",       &cli_do_param_test);
}

void cli_buf_insert(char c)
{
    if(cli_cmd_ptr == CMD_BUFF_MAX-1)
    {
        memset(cli_cmd_buff, 0, CMD_BUFF_MAX);
        cli_cmd_ptr = 0;
    }

    cli_cmd_buff[cli_cmd_ptr] = c;
    cli_cmd_ptr++;
}

void cli_prompt(void)
{
    vos_print("\r\nNIX#");
}

void cli_main_task(void)
{
    char ch;
    int rc;

    cli_prompt();
    
    while(!cmd_end_flag)    
    {
        ch = getchar();    

        if (ch == '\n')
        {
            rc = cli_cmd_exec(cli_cmd_buff);
            if(rc != VOS_OK)
            {
                if(rc == CMD_ERR_AMBIGUOUS)
                    vos_print("# ambiguous command\r\n");
                else if(rc == CMD_ERR_NOT_MATCH)
                    vos_print("# unknown command\r\n");
                else
                    vos_print("# command exec error\r\n");
            }
            
            memset(cli_cmd_buff, 0, CMD_BUFF_MAX);
            cli_cmd_ptr = 0;
            
            cli_prompt();
        }
        else
        {
            cli_buf_insert(ch);
        }
    }
}


