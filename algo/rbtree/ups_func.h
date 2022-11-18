#ifndef _DPCARD_FUNC_H_
#define _DPCARD_FUNC_H_

#define MAX_Q_NUM                   4
#define MAX_APP_NAME                128
#define IP_APP_BUCK_CNT				128
#define USER_TBL_BUCK_CNT			128
#define IP_APP_LIST_MAX				(128*1024)

#define UPS_MAGIC_NUM               0x5AA52022
#define MAX_PKT_CNT                 10240
#define MAX_PKT_SIZE                2000
#define PKT_CTRL_SIZE               48

#define PKT_STATE_IDLE              0
#define PKT_STATE_PEND              1
#define PKT_STATE_PROC              2

/********************************************************************************/
#pragma pack(1)

typedef struct {
    uint8_t  tag;
    uint8_t  len;
    char     value[0];
}ext_tag_t;

typedef struct {
    uint8_t  tag;
    uint8_t  len;
    uint64_t imsi;
}tag_imsi;

typedef struct {
    uint8_t  version;       //0
    uint8_t  pad1[12];      //1
    uint32_t sip;           //13
    uint16_t sport;         //17
    uint16_t protocol;      //19
    uint8_t  pad2[12];      //21
    uint32_t dip;           //33
    uint16_t dport;         //37
    uint8_t  tag_type;      //39
    uint8_t  len;           //40
    uint8_t  Bflag;         //41
    uint8_t  Ncode;         //42
    char     ext_tag[0];    //43
}payload_hdr_t;


#pragma pack()
/********************************************************************************/
#pragma pack(4)

typedef struct {
    uint32_t dip;
    uint64_t imsi;
}pkt_info_t;

typedef struct {
    char app_name[MAX_APP_NAME];
    uint32_t hash_id;
}app_id_t;

typedef struct {
    uint32_t ip_addr;
    uint32_t app_cnt;
    app_id_t *app_list;
}ip_app_tbl_t;

typedef struct {
    uint32_t ip_addr;
    uint32_t weight;
    uint32_t count;
}ip_list_t;

typedef struct {
    app_id_t    app_id;
	uint32_t	ip_cnt;
	gen_table_t ip_tbl;	//ip_list_t in gen_type_t
}user_app_t;

typedef struct {
    uint64_t    imsi;
	uint32_t	app_cnt;
    gen_table_t app_tbl; //user_app_t in gen_type_t
}user_imsi_t;

typedef struct {
    uint64_t err_mq_rx;
    uint64_t rx_pkts;
    uint64_t no_parse_cnt;
    uint64_t err_ip_hdr;
    uint64_t err_pl_hdr;
    uint64_t no_imsi;
	
	uint64_t dip_match_fail;
	uint64_t dip_match_ok;
    
	uint64_t imsi_match_fail;
	uint64_t imsi_add_ok;
	uint64_t imsi_add_fail;
	uint64_t imsi_no_login;     //imsi no login cos app_cnt too much
	
	uint64_t app_no_login;      //imsi no login cos app_cnt too much
	uint64_t app_add_fail;
	uint64_t app_add_ok;

	uint64_t ip_add_fail;
	uint64_t ip_add_ok;
	uint64_t ip_update_cnt;
}queue_cnt_t;

#pragma pack()
/********************************************************************************/
typedef struct {
    int task_id;
    int cpu_id;
    int msg_qid;
    int msg_type;
}task_ctrl_t;

typedef struct 
{
    long msg_type;
    void *pkt_mbuf;
    int pkt_len;
    int magic_num;
} pkt_msg_t;

typedef struct 
{
    uint64_t rd_cnt;
    uint64_t wr_cnt;
    uint32_t state;
    int resv[7];
} pkt_ctrl_t;   //less than PKT_CTRL_SIZE

/********************************************************************************/
int ups_func_init(void);
int ups_parse_pkt(int queue_id, char *pkt_data, int pkt_len);
int ups_dispatch_pkt(int queue_id, void *pkt_data, int pkt_len);
int init_pkt_proc_task(void);

int ip_app_cmp(void * a, void * b);
int user_imsi_cmp(void * a, void * b);
int user_app_cmp(void * a, void * b);
int ip_list_cmp(void * a, void * b);
int gen_random_app(ip_app_tbl_t *demo);
int insert_demo_data(int mode, int entry_cnt, int debug_flag);
int init_ip_app_table(int max_num);
int init_user_imsi_table(void);


#endif

