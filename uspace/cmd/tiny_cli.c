#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#define SERVER_PORT             2300
#define MAX_CLIENTS             5
#define MAX_CMD_LEN             512
#define MAX_HISTORY_CMDS        100
#define PROMPT                  "XP#"
#define ESC_SEQ_TIMEOUT         50000

// 特殊字符定义
#define CR                      0x0D   // ^M
#define LF                      0x0A   // 换行
#define BS                      0x08   // 退格
#define DEL                     0x7F  // 删除键
#define ESC                     0x1B  // 转义
#define IAC                     255   // Telnet协商指令

#define CMD_OK                  0x00
#define CMD_ERR                 0x01
#define CMD_ERR_PARAM           0x02
#define CMD_ERR_NOT_MATCH       0x03
#define CMD_ERR_AMBIGUOUS       0x04

typedef int (* CMD_FUNC)(int fd, int argc, char **argv);

typedef struct CMD_NODE
{
    struct CMD_NODE *pNext;

    char  *cmd_str;
    char  *help_str;
    CMD_FUNC  cmd_func;    
}CMD_NODE;

typedef struct {
    char commands[MAX_HISTORY_CMDS][MAX_CMD_LEN];
    int count;
    int current;
} CmdHistory;

typedef struct {
    int client_fd;
    CmdHistory history;
    int is_windows_client;
    char current_line[MAX_CMD_LEN];     // 当前显示的行内容
    int cursor_pos;                     // 光标位置
    int line_length;                    // 当前行的长度（缓存，避免频繁strlen）
} ClientData;


#if 1

CMD_NODE  *gst_cmd_list  = NULL;

void init_history(CmdHistory *history) 
{
    memset(history->commands, 0, sizeof(history->commands));
    history->count = 0;
    history->current = -1;
}

void add_history(CmdHistory *history, const char *cmd) 
{
    if (cmd == NULL || strlen(cmd) == 0) return;
    if (history->count > 0 && strcmp(history->commands[history->count-1], cmd) == 0) return;
    
    if (history->count >= MAX_HISTORY_CMDS) {
        for (int i = 1; i < MAX_HISTORY_CMDS; i++) {
            strncpy(history->commands[i-1], history->commands[i], MAX_CMD_LEN-1);
            history->commands[i-1][MAX_CMD_LEN-1] = '\0';
        }
        history->count--;
    }
    
    strncpy(history->commands[history->count], cmd, MAX_CMD_LEN-1);
    history->commands[history->count][MAX_CMD_LEN-1] = '\0';
    history->count++;
    history->current = history->count;
}

ssize_t safe_write(int fd, const char *buf, size_t len) 
{
    if (len == 0) len = strlen(buf);
    if (fd < 0 || buf == NULL || len == 0) return -1;
    ssize_t n;
    do {
        n = write(fd, buf, len);
    } while (n < 0 && errno == EINTR);
    return n;
}

int vos_print(int fd, const char * format,...)
{
    va_list args;
    int len;
    char print_buff[1024];

    va_start(args, format);
    len = vsnprintf(print_buff, 1024, format, args);
    va_end(args);

    safe_write(fd, print_buff, len);
    return len;    
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

int cli_do_exit(int fd, int argc, char **argv)
{
    vos_print(fd, "Goodbye!\r\n");
    close(fd);
    exit(0);
    return CMD_OK;
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

int cli_do_param_test(int fd, int argc, char **argv)
{
    vos_print(fd, "param format: \r\n");
    for (int i=0; i<argc; i++)
    {
        vos_print(fd, "%d: %s\r\n", i, argv[i]);
    }

	vos_print(fd, "char=%ld short=%ld int=%ld \r\n", sizeof(char), sizeof(short), sizeof(int));
	vos_print(fd, "ulong=%ld ptr=%ld llong=%ld \r\n", sizeof(unsigned long), sizeof(char *), sizeof(long long));
	vos_print(fd, "float=%ld double=%ld \r\n", sizeof(float), sizeof(double));
	vos_print(fd, "CPU: %s endian \r\n", get_cpu_endian()?"big":"little");
    
    return CMD_OK;
}


int cli_do_show_version(int fd, int argc, char **argv)
{
    vos_print(fd, "==============================================\r\n");
    vos_print(fd, "vos version: 1.0\r\n");
    vos_print(fd, "compile time: %s, %s\r\n", __DATE__, __TIME__);
    vos_print(fd, "==============================================\r\n");
    
    return CMD_OK;
}

int cli_do_help(int fd, int argc, char **argv)
{
    CMD_NODE *pNode;

    pNode = gst_cmd_list;
    while(pNode != NULL)
    {
        vos_print(fd, "%-24s -- %-45s \r\n", pNode->cmd_str, pNode->help_str);
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
}

#endif

#if 1

void cli_show_match_cmd(int fd, char *cmd_buf, uint32_t key_len)
{
    CMD_NODE *pNode;

    pNode = gst_cmd_list;
    while (pNode != NULL)
    {
        if(strncasecmp(pNode->cmd_str, cmd_buf, key_len) == 0)
        {
            vos_print(fd, "%-24s -- %-45s \r\n", pNode->cmd_str, pNode->help_str);
        }
        pNode = pNode->pNext;
    }
}

uint32_t cli_param_format(char *param, char **argv, uint32_t max_cnt)
{
    char *ptr = param;
    uint32_t cnt = 0;
    uint32_t flag = 0;

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

int execute_command(ClientData *client, char *buff)
{
    uint32_t  cmd_key_len;
    uint32_t  cmd_len;
    uint32_t  i;
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

    if (pNode == NULL)
    {
        //cli_run_shell(buff);
	    vos_print(client->client_fd, "unknown cmd: %s \r\n", buff);
        return CMD_OK; 
    }

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
            cli_show_match_cmd(client->client_fd, buff, cmd_key_len);
            return CMD_OK;
        }
    }

    // exec
    add_history(&client->history, buff);
    argc = cli_param_format(buff, argv, 32);
    rc = (pNode->cmd_func)(client->client_fd, argc, argv);
    
    return rc;
}

void telnet_negotiate(ClientData *client) 
{
    int fd = client->client_fd;
    
    // 只发送关键的协商命令
    unsigned char will_echo[] = {IAC, 251, 1};    // WILL ECHO
    unsigned char will_sga[] = {IAC, 251, 3};     // WILL SGA (Suppress Go Ahead)
    
    safe_write(fd, (char*)will_echo, sizeof(will_echo));
    safe_write(fd, (char*)will_sga, sizeof(will_sga));
    
    // 简单等待一下让客户端处理
    usleep(10000); // 10ms
}

// 刷新整行
void refresh_line(ClientData *client) 
{
    // 移动到行首
    safe_write(client->client_fd, "\r\033[K", 4);  // 回车 + 清除行
    
    // 显示提示符和内容
    safe_write(client->client_fd, PROMPT, strlen(PROMPT));
    safe_write(client->client_fd, client->current_line, client->line_length);
}

// 部分刷新行（从指定位置开始）
void refresh_line_partial(ClientData *client, int from_pos) 
{
    // 将光标移动到要刷新的位置
    int prompt_len = strlen(PROMPT);
    int current_pos = prompt_len + client->cursor_pos;
    int target_pos = prompt_len + from_pos;
    
    if (current_pos < target_pos) {
        // 需要向右移动
        char move_cmd[16];
        snprintf(move_cmd, sizeof(move_cmd), "\033[%dC", target_pos - current_pos);
        safe_write(client->client_fd, move_cmd, strlen(move_cmd));
    } else if (current_pos > target_pos) {
        // 需要向左移动
        char move_cmd[16];
        snprintf(move_cmd, sizeof(move_cmd), "\033[%dD", current_pos - target_pos);
        safe_write(client->client_fd, move_cmd, strlen(move_cmd));
    }
    
    // 清除从该位置到行尾的内容
    safe_write(client->client_fd, "\033[K", 3);
    
    // 重新显示从该位置开始的内容
    if (from_pos < client->line_length) {
        safe_write(client->client_fd, &client->current_line[from_pos], client->line_length - from_pos);
        
        // 如果光标不在行尾，需要移动回去
        if (client->cursor_pos < client->line_length) {
            int move_back = client->line_length - client->cursor_pos;
            if (move_back > 0) {
                char move_cmd[16];
                snprintf(move_cmd, sizeof(move_cmd), "\033[%dD", move_back);
                safe_write(client->client_fd, move_cmd, strlen(move_cmd));
            }
        }
    }
}

// 移动光标向左
void move_cursor_left(ClientData *client) 
{
    if (client->cursor_pos > 0) {
        client->cursor_pos--;
        safe_write(client->client_fd, "\033[D", 3); // 向左移动一个字符
    }
}

// 移动光标向右
void move_cursor_right(ClientData *client) 
{
    if (client->cursor_pos < client->line_length) {
        client->cursor_pos++;
        safe_write(client->client_fd, "\033[C", 3); // 向右移动一个字符
    }
}

// 在光标处插入字符
void insert_char_at_cursor(ClientData *client, char c) 
{
    if (client->line_length < MAX_CMD_LEN - 1) {
        // 如果光标不在末尾，需要先进入插入模式
        if (client->cursor_pos < client->line_length) {
            // 将光标位置及之后的字符向右移动
            for (int i = client->line_length; i > client->cursor_pos; i--) {
                client->current_line[i] = client->current_line[i-1];
            }
            client->current_line[client->cursor_pos] = c;
            client->line_length++;
            
            // 显示插入的字符及其后面的字符
            safe_write(client->client_fd, &client->current_line[client->cursor_pos], 
                      client->line_length - client->cursor_pos);
            
            // 光标回到正确位置
            if (client->cursor_pos < client->line_length - 1) {
                int move_back = client->line_length - client->cursor_pos - 1;
                if (move_back > 0) {
                    char move_cmd[16];
                    snprintf(move_cmd, sizeof(move_cmd), "\033[%dD", move_back);
                    safe_write(client->client_fd, move_cmd, strlen(move_cmd));
                }
            }
        } else {
            // 光标在末尾，直接追加
            client->current_line[client->cursor_pos] = c;
            client->line_length++;
            client->current_line[client->line_length] = '\0';
            safe_write(client->client_fd, &c, 1);
        }
        client->cursor_pos++;
    }
}

// 删除光标处的字符
void delete_char_at_cursor(ClientData *client) 
{
    if (client->cursor_pos < client->line_length) {
        // 将光标位置之后的字符向左移动
        for (int i = client->cursor_pos; i < client->line_length; i++) {
            client->current_line[i] = client->current_line[i+1];
        }
        client->line_length--;
        
        // 从光标位置开始刷新行
        refresh_line_partial(client, client->cursor_pos);
    }
}

// 删除光标前的字符（退格键）
void delete_char_before_cursor(ClientData *client) 
{
    if (client->cursor_pos > 0) {
        // 移动光标并删除
        move_cursor_left(client);
        delete_char_at_cursor(client);
    }
}

// 关键修复：读取字符时过滤CR，转换为内部标记
int read_char_with_cr_filter(int fd, char *c) 
{
    if (fd < 0 || c == NULL) return -1;
    
    while (1) {
        ssize_t n = read(fd, c, 1);
        if (n <= 0) return n;
        
        // 跳过Telnet协商指令
        if (*c == IAC) {
            char dummy[2];
            read(fd, dummy, 2);
            continue;
        }
        
        // 关键：将CR转换为LF，作为内部回车标记
        if (*c == CR) {
            *c = LF;  // 转换为LF，作为内部回车标记
            return n;
        }
        
        return n;
    }
}

// 处理ESC序列（上下左右箭头）
int handle_esc_sequence(int fd) 
{
    char seq[3] = {0};
    
    // 使用select设置超时
    struct timeval tv = {0, ESC_SEQ_TIMEOUT};
    
    // 读取'['
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    
    int ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret <= 0) return 0;
    
    if (read(fd, &seq[0], 1) != 1 || seq[0] != '[') return 0;
    
    // 读取方向键
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_usec = ESC_SEQ_TIMEOUT;
    
    ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret <= 0) return 0;
    
    if (read(fd, &seq[1], 1) != 1) return 0;
    
    // 可能是'A', 'B', 'C', 'D' 分别对应上、下、右、左
    return seq[1];
}

char* readline_with_history(ClientData *client) 
{
    static char input_buf[MAX_CMD_LEN];
    int fd = client->client_fd;
    
    // 重置
    memset(input_buf, 0, sizeof(input_buf));
    memset(client->current_line, 0, sizeof(client->current_line));
    client->cursor_pos = 0;
    client->line_length = 0;
    client->history.current = client->history.count;
    
    // 初始显示
    safe_write(fd, PROMPT, strlen(PROMPT));
    
    while (1) {
        char c;
        int n = read_char_with_cr_filter(fd, &c);
        if (n <= 0) return NULL;
        
        // 处理ESC序列
        if (c == ESC) {
            int arrow = handle_esc_sequence(fd);
            if (arrow == 'A') { // 上箭头
                char *new_cmd = NULL;
                if (client->history.count > 0 && client->history.current > 0) {
                    client->history.current--;
                    new_cmd = client->history.commands[client->history.current];
                }
                
                if (new_cmd != NULL) {
                    strncpy(input_buf, new_cmd, MAX_CMD_LEN-1);
                    input_buf[MAX_CMD_LEN-1] = '\0';
                    strncpy(client->current_line, input_buf, MAX_CMD_LEN-1);
                    client->line_length = strlen(input_buf);
                    client->cursor_pos = client->line_length;
                    refresh_line(client);
                }
                continue;
            } else if (arrow == 'B') { // 下箭头
                char *new_cmd = NULL;
                if (client->history.count > 0) {
                    if (client->history.current < client->history.count - 1) {
                        client->history.current++;
                        new_cmd = client->history.commands[client->history.current];
                    } else {
                        new_cmd = "";
                        client->history.current = client->history.count;
                    }
                }
                
                if (new_cmd != NULL) {
                    strncpy(input_buf, new_cmd, MAX_CMD_LEN-1);
                    input_buf[MAX_CMD_LEN-1] = '\0';
                    strncpy(client->current_line, input_buf, MAX_CMD_LEN-1);
                    client->line_length = strlen(input_buf);
                    client->cursor_pos = client->line_length;
                    refresh_line(client);
                }
                continue;
            } else if (arrow == 'C') { // 右箭头
                move_cursor_right(client);
                continue;
            } else if (arrow == 'D') { // 左箭头
                move_cursor_left(client);
                continue;
            }
            // 其他ESC序列忽略
            continue;
        }
        
        // 处理回车（CR已转换为LF）
        if (c == LF) {
            // 关键：发送CRLF进行换行，不显示^M
            safe_write(fd, "\r\n", 2);
            
            // 复制当前行到输入缓冲区
            strncpy(input_buf, client->current_line, MAX_CMD_LEN-1);
            input_buf[MAX_CMD_LEN-1] = '\0';
            
            // 空命令处理
            if (strlen(input_buf) == 0) {
                safe_write(fd, PROMPT, strlen(PROMPT));
                memset(client->current_line, 0, sizeof(client->current_line));
                client->cursor_pos = 0;
                client->line_length = 0;
                continue;
            }
            
            return input_buf;
        }
        
        // 处理退格和删除
        if (c == BS || c == DEL) {
            delete_char_before_cursor(client);
            continue;
        }
        
        // 处理制表符
        if (c == '\t') {
            insert_char_at_cursor(client, ' ');
            continue;
        }
        
        // 处理普通字符
        if (c >= 0x20 && c <= 0x7E) {
            insert_char_at_cursor(client, c);
            continue;
        }
        
        // 其他控制字符忽略
    }
}

void handle_client(int client_fd) 
{
    ClientData client;
    client.client_fd = client_fd;
    client.is_windows_client = 0;
    client.cursor_pos = 0;
    client.line_length = 0;
    init_history(&client.history);
    memset(client.current_line, 0, sizeof(client.current_line));

    telnet_negotiate(&client);
    safe_write(client_fd, "\r\nWelcome to Telnet Server\r\n", 28);

    while (1) {
        char *cmd = readline_with_history(&client);
        if (cmd == NULL) break;
        execute_command(&client, cmd);
    }
    close(client_fd);
}

int main() 
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(1);
    }

    printf("Telnet server running on port %d\n", SERVER_PORT);
    cli_cmd_init();
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if (fork() == 0) {
            close(server_fd);
            handle_client(client_fd);
            exit(0);
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}

#endif

