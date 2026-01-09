#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
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
#include <sched.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>



int get_local_ip(char *if_name)
{
    int inet_sock;  
    struct ifreq ifr;  

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);  
    strcpy(ifr.ifr_name, if_name);  
    if ( ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0 ) {
		close(inet_sock);
        return -1;
    }

	close(inet_sock);
    return (int)((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
}

int get_all_ips_for_interface(const char *if_name) 
{
    struct ifaddrs *ifaddr_list, *ifa;
    int family;
    int count = 0;

    // 1. 获取所有接口地址链表
    if (getifaddrs(&ifaddr_list) == -1) {
        perror("getifaddrs failed");
        return -1;
    }

    printf("All IPv4 addresses for interface '%s':\n", if_name);

    // 2. 遍历链表
    for (ifa = ifaddr_list; ifa != NULL; ifa = ifa->ifa_next) {
        // 跳过空地址指针或接口名不匹配的项
        if (ifa->ifa_addr == NULL || strcmp(ifa->ifa_name, if_name) != 0) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        // 3. 只处理IPv4地址 (AF_INET)
        if (family == AF_INET) { 
            count++;
            // 转换二进制IP为可读字符串
            char ip_str[INET_ADDRSTRLEN];
            struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));

            // 4. 打印地址信息
            printf("\t%d. %s\n", count, ip_str);
            // 你可以在这里将IP地址存储到数组或链表中，而不仅仅是打印
        }
        // 你也可以类似地处理 IPv6 (AF_INET6)
    }

    // 5. 释放链表
    freeifaddrs(ifaddr_list);

    if (count == 0) {
        printf("\t(No IPv4 addresses found for this interface)\n");
    }
    return count;
}


int main(int argc, char **argv)
{
    struct in_addr addr;

    if (argc < 2) {
        printf("%s <netdev> \n", argv[0]);
        return 0;
    }
    
    get_all_ips_for_interface(argv[1]);
    //addr.s_addr = get_local_ip(argv[1]);
    //printf("ip address %s \r\n", inet_ntoa(addr));
    return 0;    
}

