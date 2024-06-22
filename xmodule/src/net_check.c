#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAGIC 					"1234567890"
#define MAGIC_LEN 				11
#define MTU 					1500
#define RECV_TIMEOUT_USEC 		500000
#define SEND_INTERVAL			(0.5)

#ifdef MAKE_APP
#define my_debug                printf
#else
#define my_debug(...)
#endif

struct icmp_echo {
    // header
    uint8_t type;
    uint8_t code;
    uint16_t checksum;

    uint16_t ident;
    uint16_t seq;

    // data
    double sending_ts;
    char magic[MAGIC_LEN];
};

double get_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

uint16_t calculate_checksum(unsigned char* buffer, int bytes)
{
    uint32_t checksum = 0;
    unsigned char* end = buffer + bytes;

    // odd bytes add last byte and reset end
    if (bytes % 2 == 1) {
        end = buffer + bytes - 1;
        checksum += (*end) << 8;
    }

    // add words of two bytes, one by one
    while (buffer < end) {
        checksum += buffer[0] << 8;
        checksum += buffer[1];
        buffer += 2;
    }

    // add carry if any
    uint32_t carray = checksum >> 16;
    while (carray) {
        checksum = (checksum & 0xffff) + carray;
        carray = checksum >> 16;
    }

    // negate it
    checksum = ~checksum;
    return checksum & 0xffff;
}

int send_echo_request(int sock, struct sockaddr_in* addr, int ident, int seq)
{
    struct icmp_echo icmp;
	
    bzero(&icmp, sizeof(icmp));
    icmp.type = 8;
    icmp.code = 0;
    icmp.ident = htons(ident);
    icmp.seq = htons(seq);

    strncpy(icmp.magic, MAGIC, MAGIC_LEN);
    icmp.sending_ts = get_timestamp();
    icmp.checksum = htons(calculate_checksum((unsigned char*)&icmp, sizeof(icmp)));

    int bytes = sendto(sock, &icmp, sizeof(icmp), 0, (struct sockaddr*)addr, sizeof(*addr));
    if (bytes < 0) {
        return -1;
    }

    my_debug("send %d bytes, ident %d, seq %d \n", bytes, ident, seq);
    return 0;
}

int recv_echo_reply(int sock, int ident)
{
    char buffer[MTU];
    struct sockaddr_in peer_addr;
    int addr_len = sizeof(peer_addr);

    int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&peer_addr, &addr_len);
    if (bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
			//perror("EAGAIN");
            return -1;
        }
		my_debug("recvfrom");
        return -1;
    }

    struct icmp_echo* icmp = (struct icmp_echo*)(buffer + 20);
    if (icmp->type != 0 || icmp->code != 0) {
		//my_debug("type %d, code %d \n", icmp->type, icmp->code);
        return -1;
    }

    ident = ident & 0xFFFF;
    if ((ntohs(icmp->ident) != ident) && (icmp->ident != ident)) {
		my_debug("ident 0x%x, 0x%x \n", icmp->ident, ident);
        return -1;
    }

    my_debug("%s seq=%d %5.2fms\n", inet_ntoa(peer_addr.sin_addr), ntohs(icmp->seq), 
    	     (get_timestamp() - icmp->sending_ts) * 1000);

    return 0;
}

int ping_check(const char *ip, int cnt)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    if (inet_aton(ip, (struct in_addr*)&addr.sin_addr.s_addr) == 0) {
		my_debug("inet_aton failed \n");
        return -1;
    };

    // create raw socket for icmp protocol
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == -1) {
		my_debug("socket failed \n");
        return -1;
    }

    // set socket timeout option
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RECV_TIMEOUT_USEC;
    int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret == -1) {
		my_debug("setsockopt failed \n");
        return -1;
    }

    double next_ts = get_timestamp();
    int ident = getpid();
    int seq = 1;
	int ok_cnt = 0;
    while (cnt > 0) {
        if (get_timestamp() >= next_ts) {
            ret = send_echo_request(sock, &addr, ident, seq++);
            if (ret == -1) my_debug("Send failed");
			cnt--; //even fail

            // update next send timestamp
            next_ts += SEND_INTERVAL;
        } 

        usleep(200000);
		ret = recv_echo_reply(sock, ident);
		if (ret == 0) ok_cnt++;
    }

    return ok_cnt;
}

#ifdef MAKE_APP
int main(int argc, char **argv)
{	
    int tx, rx;
    
	if (argc < 3) {
        printf("usage: %s <ip_addr> <cnt> \n", argv[0]);
        return 0;
    }

    tx = atoi(argv[2]);
    rx = ping_check(argv[1], tx);
    printf("tx %d, rx %d \n", tx, rx);
    return 0;
}
#endif

