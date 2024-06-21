#include "vos.h"
#include "tiny_cli.h"

char    cli_cmd_buff[CMD_BUFF_MAX];

uint32  cli_cmd_ptr = 0;

int     telnet_fd = -1;

int     cli_console_active = -1;

CMD_NODE  *gst_cmd_list  = NULL;

uint32     pwd_check_ok = FALSE;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

char print_buff[8192];

CLI_OUT_CB cli_out = NULL;

void *cb_cookie = NULL;

int cli_telnet_active(void)
{
    return telnet_fd >= 0;
}

int cli_set_output_cb(CLI_OUT_CB cb, void *cookie)
{
    cli_out = cb;
    cb_cookie = cookie;
    return CMD_OK;
}

int vos_print(const char * format,...)
{
    va_list args;
    int len;

    pthread_mutex_lock(&print_mutex);
    va_start(args, format);
    len = vsnprintf(print_buff, 8192, format, args);
    va_end(args);

    if (cli_out != NULL) {
        cli_out(cb_cookie, print_buff);
    } else if ( cli_telnet_active() ) {
        write(telnet_fd, print_buff, len);
    } else {
        printf("%s", print_buff);
		fflush(stdout);
    }

    pthread_mutex_unlock(&print_mutex);
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

    //get the last param
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

void cli_show_match_cmd(char *cmd_buf, uint32 key_len)
{
    CMD_NODE *pNode;

    pNode = gst_cmd_list;
    while (pNode != NULL)
    {
        if(strncasecmp(pNode->cmd_str, cmd_buf, key_len) == 0)
        {
            vos_print("%-24s -- %-45s \r\n", pNode->cmd_str, pNode->help_str);
        }
        pNode = pNode->pNext;
    }
}

#ifdef INCLUDE_SHELL_CMD    

int shell_enable = 0;

#define OUTPUT_TEMP_FILE   "/run/cmd.log" 

int cli_run_shell(char *cmd_buf)
{
    int ret;
    FILE *fp;
	char temp_buf[512];

    //vos_print("cmd: %s \r\n", cmd_buf);
    ret = vos_run_cmd("%s > %s 2>&1", cmd_buf, OUTPUT_TEMP_FILE);
    if (ret < 0) {
        vos_print("cmd failed \r\n");
        return 0;
    } 

    fp = fopen(OUTPUT_TEMP_FILE, "r");
    if (fp == NULL) {
        vos_print("cmd failed \r\n");
        return CMD_ERR;
    }

    memset(temp_buf, 0, sizeof(temp_buf));
    while (fgets(temp_buf, 500, fp) != NULL) {  
        vos_print("%s\r", temp_buf); //linux-\n, windows-\n\r
        memset(temp_buf, 0, sizeof(temp_buf));
    }

    fclose(fp);
    unlink(OUTPUT_TEMP_FILE);    
    return CMD_OK;
}

#endif

#if 1

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
        return CMD_OK;
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
        if(strncasecmp(pNode->cmd_str, buff, cmd_key_len) == 0)
        {
            break;
        }
        pNode = pNode->pNext;
    }

#ifdef CLI_PWD_CHECK
    if (pwd_check_ok != TRUE) {
        if ( (strncasecmp("passwd", buff, 6) != 0)  &&
             (strncasecmp("quit", buff, 4) != 0) ){
            vos_print("input 'passwd' to verify password, or input 'quit' to exit \r\n");
            return CMD_OK; 
        }
    }
#endif

    if (pNode == NULL)
    {
        #ifdef INCLUDE_SHELL_CMD 
        if (strncasecmp("sh_enable", buff, 9) == 0) {
            shell_enable = 1;
        } else if ( shell_enable ) {
        	cli_run_shell(buff);
        } else {
	        vos_print("unknown cmd: %s \r\n", buff);
        }
        #else
        vos_print("unknown cmd: %s \r\n", buff);
        #endif
        return CMD_OK; 
    }

#ifdef CHECK_AMBIGUOUS
    //if not full match, check ambiguous
    if(cmd_key_len < strlen(pNode->cmd_str))
    {
        CMD_NODE *iNode;
        iNode = pNode->pNext;
        while(iNode != NULL)
        {
            if(strncasecmp(iNode->cmd_str, buff, cmd_key_len) == 0)
            {
                break;
            }
            iNode = iNode->pNext;
        }   
        
        if(iNode != NULL)
        {
            cli_show_match_cmd(buff, cmd_key_len);
            return CMD_OK;
        }
    }
#endif    

    // exec
    argc = cli_param_format(buff, argv, 32);
    rc = (pNode->cmd_func)(argc, argv);
    
    return rc;
}

int cli_cmd_reg(const char *cmd, const char *help, CMD_FUNC func)
{
    CMD_NODE *new_node;
    CMD_NODE *p, *q;
    
    new_node = (CMD_NODE *)malloc(sizeof(CMD_NODE));
    if(new_node == NULL)
    {
        printf("VOS_MALLOC failed!\r\n");
        return CMD_ERR;
    }

    memset(new_node, 0, sizeof(CMD_NODE));
    new_node->cmd_func = func;
    new_node->cmd_str = (char *)strdup(cmd);
    new_node->help_str = (char *)strdup(help);
    new_node->pNext = NULL;

    //printf("cli_cmd_reg: %s(%s) \r\n", new_node->cmd_str, new_node->help_str);
    q = NULL;
    p = gst_cmd_list;
    while (p != NULL) {
        if (strcmp(p->cmd_str, new_node->cmd_str) > 0) {
            if (q == NULL) { //add to head
                new_node->pNext = p;
                gst_cmd_list = new_node;
            } else { //q -> new_node -> p
                q->pNext = new_node;
                new_node->pNext = p;
            }
            return CMD_OK;
        }
        q = p;
        p = p->pNext;
    }

    if (q != NULL) { //add to tail
        q->pNext = new_node;
    } else { //first node
        gst_cmd_list = new_node;
    }

    return CMD_OK;
}

int cli_do_spec_char(char c)
{
    if (c == '\b') {
        //printf("recv backspace\n");
        if (cli_cmd_ptr > 0) {
            cli_cmd_ptr--;
            cli_cmd_buff[cli_cmd_ptr] = 0;
            vos_print("\b");
            return TRUE;
        }
    } else if (c == 0xFF) {
    	sleep(1);
		return TRUE;
    }
    
    return FALSE;   //not special char
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
    vos_print("\r\nvos>");
}

void* cli_main_task(void *param)
{
    char ch;
    int ret;

    cli_prompt();
	cli_console_active = 1;
    while(1)    
    {
        if (cli_telnet_active()) {
            sleep(1);
            continue;
        }
        
        ch = getchar();
        if ( (ch == '\r') || (ch == '\n') ) {
            ret = cli_cmd_exec(cli_cmd_buff);
            if(ret != CMD_OK){
                if (ret == CMD_ERR_NOT_MATCH) vos_print("unknown command\r\n");
                else if (ret == CMD_ERR_EXIT) break;
                else vos_print("command exec error\r\n");
            }
            
            memset(cli_cmd_buff, 0, CMD_BUFF_MAX);
            cli_cmd_ptr = 0;
            cli_prompt();
        } 
		else if (cli_do_spec_char(ch)) {
            //null
        }
        else {
            cli_buf_insert(ch);
        }
    }

	cli_console_active = 0;
	exit(0);
    return NULL;
}

int cli_task_init(void)
{
    int ret;
    pthread_t unused_tid;

    ret = pthread_create(&unused_tid, NULL, cli_main_task, NULL);  
    if (ret != 0)  {  
        printf("Error at %s:%d, pthread_create failed(%s)\r\n", __FILE__, __LINE__, strerror(errno));
        return CMD_ERR;  
    } 
    return CMD_OK;
}

int cli_task_run(void)
{
	if (telnet_fd > 0 || cli_console_active)
		return 1;
	return 0;
}

#endif

#if 1

int cli_do_exit(int argc, char **argv)
{
    vos_print("exit cmd ... \r\n");
    return CMD_ERR_EXIT;
}

int get_cpu_endian() 
{
	union {
		int number;
		char s;
	} test;

    test.number = 0x01000002;
    return (test.s == 0x01);
}

int cli_do_param_test(int argc, char **argv)
{
    vos_print("param format: \r\n");
    for (int i=0; i<argc; i++)
    {
        vos_print("%d: %s\r\n", i, argv[i]);
    }
    
    return CMD_OK;
}

int cli_do_sz_check(int argc, char **argv)
{
	vos_print("char=%ld short=%ld int=%ld \r\n", sizeof(char), sizeof(short), sizeof(int));
	vos_print("ulong=%ld ptr=%ld llong=%ld \r\n", sizeof(unsigned long), sizeof(char *), sizeof(long long));
	vos_print("float=%ld double=%ld \r\n", sizeof(float), sizeof(double));
	vos_print("CPU: %s endian \r\n", get_cpu_endian()?"big":"little");
    
    return CMD_OK;
}

int cli_do_passwd_verify(int argc, char **argv)
{
    if (argc < 2) {
        vos_print("usage: %s <passwd_str> \r\n", argv[0]);
        return CMD_OK;
    }

    if (memcmp("simpie", argv[1], 6) != 0) {  //todo
        vos_print("invalid password \r\n");
        return CMD_OK;
    }

    pwd_check_ok = TRUE;
    vos_print("password verified OK \r\n");
    return CMD_OK;
}

int cli_do_show_version(int argc, char **argv)
{
    vos_print("==============================================\r\n");
    vos_print("vos version: 1.0\r\n");
    vos_print("compile time: %s, %s\r\n", __DATE__, __TIME__);
    vos_print("==============================================\r\n");
    
    return CMD_OK;
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

    return CMD_OK;
}

void cli_cmd_init(void)
{
    cli_cmd_reg("quit",         "exit app",             &cli_do_exit);
    cli_cmd_reg("help",         "cmd help",             &cli_do_help);
    cli_cmd_reg("version",      "show version",         &cli_do_show_version);
    cli_cmd_reg("cmdtest",      "cmd param test",       &cli_do_param_test);
    cli_cmd_reg("xsize",        "xsize check",       	&cli_do_sz_check);
	
#ifdef CLI_PWD_CHECK    
    cli_cmd_reg("passwd",       "password verify",      &cli_do_passwd_verify);
#endif
}
#endif

#ifdef INCLUDE_TELNETD

void cli_telnet_task(int fd)
{
    char buf[CMD_BUFF_MAX];
    char ch;
    int len, ret;

    if (cli_telnet_active()) {
        return ;
    }

#ifdef CLI_PWD_CHECK
    pwd_check_ok = FALSE;
#else
    pwd_check_ok = TRUE;
#endif

    telnet_fd = fd;
    cli_prompt();
    while(1)    
    {
        //cli_prompt();
        len  = read(telnet_fd, buf, CMD_BUFF_MAX);
        if (len <= 0) { 
            break;
        }

        for (int i = 0; i < len; i++) {
            ch = buf[i];
            if ( (ch == '\r') || (ch == '\n') ) {
                vos_print("\r\n");
                ret = cli_cmd_exec(cli_cmd_buff);
                if (ret != CMD_OK) {
                    if (ret == CMD_ERR_NOT_MATCH) vos_print("unknown command\r\n");
                    else if (ret == CMD_ERR_EXIT) goto TASK_EXIT;
                    else vos_print("command exec error\r\n");
                }
                
                memset(cli_cmd_buff, 0, CMD_BUFF_MAX);
                cli_cmd_ptr = 0;
                cli_prompt();
            } else if (cli_do_spec_char(ch)) {
                //null
            }
            else if (ch > 0x1f && ch < 0x7f){
                cli_buf_insert(ch);
                if ( (!pwd_check_ok) && (cli_cmd_ptr > 7) ) {
                    vos_print("*");
                } else {
                    vos_print("%c", ch);
                }
            }
        }
    }
    
TASK_EXIT:    
    memset(cli_cmd_buff, 0, CMD_BUFF_MAX);
    cli_cmd_ptr = 0;
    telnet_fd = -1;
}

void* telnet_listen_task(void *param)  
{
	struct sockaddr_in sa;
	int master_fd;
	int on = 1;

	master_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (master_fd < 0) {
		perror("socket");
		return NULL;
	}
	(void)setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	(void)setsockopt(master_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

	/* Set it to listen to specified port */
	memset((void *)&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(TELNETD_LISTEN_PORT);

	/* Set it to listen on the specified interface */
	//sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(master_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("bind");
		return NULL;
	}

	if (listen(master_fd, 1) < 0) {
		perror("listen");
		return NULL;
	}

    //if (daemon(0, 1) < 0) perror("daemon");
    vos_set_self_name("telnet_listen_task");
    while (1) {
        int fd;
        socklen_t salen;

        salen = sizeof(sa); 
        if ((fd = accept(master_fd, (struct sockaddr *)&sa, &salen)) < 0) {
            perror("accept");
            continue;
        } else {
            printf("Server: connect from host %s, port %d.\r\n", inet_ntoa (sa.sin_addr), ntohs (sa.sin_port));        
            cli_telnet_task(fd);
            close(fd);
        }
    }

    return NULL;
}

int telnet_task_init(void)
{
    int ret;
    pthread_t unused_tid;

    ret = pthread_create(&unused_tid, NULL, telnet_listen_task, NULL);  
    if (ret != 0)  {  
        printf("Error at %s:%d, pthread_create failed(%s)\r\n", __FILE__, __LINE__, strerror(errno));
        return CMD_ERR;  
    } 
    return CMD_OK;
}

#endif


