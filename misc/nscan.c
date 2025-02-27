#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

typedef struct {
    int arfcn;
    int pci;
    int mcc;
    int mnc;

    //LTE_QS
	int lac;
    int band;
    int cellid;
    int dbm;

    //NR_QS
    int tac;
    long nci;
    int rsrp;
    int rsrq;
}FS_ENTRY_S;

//mcc:460,mnc:00,lac:9341,band:40,arfcn:38950,cellid:213455234,dBm:-73,pci:130
int fs_get_lte_info(char *line_buff)
{
    int ret;
    FS_ENTRY_S ne;

    //"http://author:zht/Time:2017-06-25/Content:hello,world/";
    //sscanf(szURL, "%*[^:]:%*[^:]:%[^/]/%*[^:]:%d-%d-%d/%*[^:]:%[^/]/", szAuthor, &year, &month, &day, szContent);
    memset(&ne, 0, sizeof(ne));
    ret = sscanf(line_buff, 
                "%*[^:]:%d,%*[^:]:%d,%*[^:]:%d,%*[^:]:%d,%*[^:]:%d,%*[^:]:%d,%*[^:]:%d,%*[^:]:%d", 
                &ne.mcc, &ne.mnc, &ne.lac, &ne.band, &ne.arfcn,
                &ne.cellid,  &ne.dbm,  &ne.pci);
    if (ret < 8) {
        printf("sscanf failed\n");
        return -1;
    }

    printf("arfcn %d, pci %d \n", ne.arfcn, ne.pci);
    return 0;
}

////5G数据格式<mnc>,<mcc>,<tac>,<nci>,<pci>,<arfcn>,<rsrp>,<rsrq>
//[nr_cell]460,11,0x768a0b,31700168980,591,633984,-1154,-140
int fs_get_nr_info(char *line_buff)
{
    int ret;
    FS_ENTRY_S ne;
    char tac_str[32];

    memset(&ne, 0, sizeof(ne));
    ret = sscanf(line_buff, 
                "%*[^0-9]%d,%d,%[^,],%ld,%d,%d,%d,%d", 
                &ne.mnc, &ne.mcc, tac_str, &ne.nci, &ne.pci,
                &ne.arfcn,  &ne.rsrp,  &ne.rsrq);
    if (ret < 8) {
        printf("sscanf failed\n");
        return -1;
    }

    ne.tac = strtoul(tac_str, 0, 0);
    printf("arfcn %d, pci %d, tac 0x%x, rsrq %d \n", 
		   ne.arfcn, ne.pci, ne.tac, ne.rsrq);
    return 0;
}

int main()
{
	fs_get_lte_info("mcc:460,mnc:00,lac:9341,band:40,arfcn:38950,cellid:213455234,dBm:-73,pci:130");	
	fs_get_nr_info("[nr_cell]460,11,0x768a0b,31700168980,591,633984,-1154,-140");

    return 0;
}