
#include "xlib.h"

#define VOS_OK          0
#define VOS_ERR         1
#define VOS_NULL        0

#ifndef uint32
#define uint32 unsigned int
#endif

#if 1
typedef int (* FUNC_ENTRY)(void *param);

typedef struct CMD_NODE
{
    struct CMD_NODE *pNext;

    char  *cmd_str;
    char  *help_str;
    FUNC_ENTRY  cmd_func;    
}CMD_NODE;

#define MAX_CMD_BUFF_SZ         1024

#define CMD_ERR_NOT_MATCH       0x0001
#define CMD_ERR_AMBIG           0x0002 //ambiguous

int vos_print(const char * format,...)
{
    va_list args;
    char buf[MAX_CMD_BUFF_SZ];
    int len;

    va_start(args, format);
    len = vsprintf(buf, format, args);
    va_end(args);

    if (len < 0 || len > 1024)
    {
        return -1;
    }

    printf("%s", buf);

    return len;    
}

char    cli_cmd_buff[MAX_CMD_BUFF_SZ];
uint32  cli_cmd_ptr = 0;

CMD_NODE  *gst_cmd_list  = VOS_NULL;
uint32     cmd_end_flag  = 0;

int cli_cmd_exec(char *buff)
{
    uint32 cmd_key_len;
    uint32 cmd_len;
    uint32 i;
    int  rc;
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

    //vos_semTake(cmd_list_sem, 0);
    pNode = gst_cmd_list;
    while(pNode != VOS_NULL)
    {
        if(strncmp(pNode->cmd_str, buff, cmd_key_len) == 0)
        {
            break;
        }
        pNode = pNode->pNext;
    }

    if(pNode == VOS_NULL)
    {
        //vos_semGive(cmd_list_sem);    
        rc = system(buff);
        return rc; // CMD_ERR_NOT_MATCH;
    }

#if 0 //todo
    //if not full match, check ambiguous
    if(cmd_key_len < vos_strlen(pNode->cmd_str))
    {
        CMD_NODE *iNode;
        iNode = pNode->pNext;
        while(iNode != VOS_NULL)
        {
            if(vos_strncmp(iNode->cmd_str, buff, cmd_key_len) == 0)
            {
                break;
            }
            iNode = iNode->pNext;
        }   
        
        if(iNode != VOS_NULL)
        {
            vos_semGive(cmd_list_sem); 
            return CMD_ERR_AMBIG;
        }
    }
#endif    

    // exec
    rc = (pNode->cmd_func)(buff);
    //vos_semGive(cmd_list_sem);    
    
    return rc;
}

int cli_cmd_reg(char *cmd, char *help, FUNC_ENTRY func)
{
    CMD_NODE *new_node;
    
    new_node = (CMD_NODE *)malloc(sizeof(CMD_NODE));
    if(new_node == VOS_NULL)
    {
        printf("VOS_MALLOC failed!\r\n");
        return VOS_ERR;
    }

    memset(new_node, 0, sizeof(CMD_NODE));
    new_node->cmd_func = func;
    new_node->cmd_str = (char *)strdup(cmd);
    new_node->help_str = (char *)strdup(help);

    printf("=====================================\r\n");
    printf("cli_cmd_reg: %s(%s) in ADDR(0x%x)\r\n", 
           new_node->cmd_str, new_node->help_str, new_node->cmd_func );
    printf("=====================================\r\n\r\n");
    
    //vos_semTake(cmd_list_sem, 0);
    new_node->pNext = gst_cmd_list;
    gst_cmd_list = new_node;
    //vos_semGive(cmd_list_sem);

    return VOS_OK;
}

int cli_do_exit(void *param)
{
    cmd_end_flag = 1;
    
    return VOS_OK;
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

int cli_do_param_test(void *param)
{
    uint32 rc, i;
    char   *argv[20];
    char   *buff;

    vos_print("param format: \r\n");
    buff = (char *)strdup(param);
    rc = cli_param_format(buff, argv, 20);
    for(i=0; i<rc; i++)
    {
        vos_print("%d: %s\r\n", i, argv[i]);
    }
    free(buff);
    
    return VOS_OK;
}

int cli_do_show_version(void *param)
{
    vos_print("===================================\r\n");
    vos_print("vos version: 1.0\r\n");
    vos_print("===================================\r\n");
    
    return VOS_OK;
}

int cli_do_help(void *param)
{
    CMD_NODE *pNode;
    
    pNode = gst_cmd_list;
    while(pNode != VOS_NULL)
    {
        vos_print("%-15s -- %-45s [0x%x]\r\n",
                  pNode->cmd_str, pNode->help_str, pNode->cmd_func);
        pNode = pNode->pNext;
    }

    return VOS_OK;
}

void cli_cmd_init(void)
{
    gst_cmd_list = VOS_NULL;
    //cmd_list_sem = _vos_semBCreateEx(0, SEM_ST_FULL, 0);
    
    cli_cmd_reg("quit",         "exit vos",         &cli_do_exit);
    cli_cmd_reg("help",         "cmd help",         &cli_do_help);
    cli_cmd_reg("version",      "show vos version", &cli_do_show_version);
    cli_cmd_reg("cmdtest",      "cmd param test",   &cli_do_param_test);
}

void cli_buf_insert(char c)
{
    if(cli_cmd_ptr == MAX_CMD_BUFF_SZ-1)
    {
        memset(cli_cmd_buff, 0, MAX_CMD_BUFF_SZ);
        cli_cmd_ptr = 0;
    }

    cli_cmd_buff[cli_cmd_ptr] = c;
    cli_cmd_ptr++;
}

void cli_prompt(void)
{
    vos_print("\r\nNIX#");
}

void cli_read_to_exec(void)
{
    char ch;
    int rc;

    cli_prompt();
    
    while(!cmd_end_flag)    
    {
        scanf("%c", &ch);    

        if(ch == '\n')
        {
            rc = cli_cmd_exec(cli_cmd_buff);
            if(rc != VOS_OK)
            {
                if(rc == CMD_ERR_AMBIG)
                    vos_print("# ambiguous command\r\n");
                else if(rc == CMD_ERR_NOT_MATCH)
                    vos_print("# unknown command\r\n");
                else
                    vos_print("# command exec error\r\n");
            }
            
            memset(cli_cmd_buff, 0, MAX_CMD_BUFF_SZ);
            cli_cmd_ptr = 0;
            
            cli_prompt();
        }
        else
        {
            cli_buf_insert(ch);
        }
    }
}

int cli_init(void)
{
    //cmd init
    cli_cmd_init();

    //user reg here
    //cli_reg_user_cmd();

    //cli read loop
    cli_read_to_exec();

	return VOS_OK;
}

int cli_term(void)
{
	return VOS_OK;
}
#endif


#if 1
int main(int argc, char **argv)
{
    cli_init();

    return 0;
}
#endif
