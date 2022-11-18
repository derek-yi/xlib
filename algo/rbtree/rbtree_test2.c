#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>   //time_t
#include <sys/time.h>
#include <stdarg.h>
#include <stddef.h>

#include "common.h"
#include "ups_func.h"


int my_cmp(void * a, void * b)
{
    ip_app_tbl_t *tmp1 = (ip_app_tbl_t *)a;
    ip_app_tbl_t *tmp2 = (ip_app_tbl_t *)b;
    return tmp1->ip_addr - tmp2->ip_addr;
}

#define IP_APP_BUCK_SZ		128

gen_table_t ip_app_bucket[IP_APP_BUCK_SZ];

int gen_random_app(ip_app_tbl_t *demo)
{
    return VOS_OK;
}

int insert_demo_data(int mode, int entry_cnt, int debug_flag)
{
	struct gen_type *entry_head;
    ip_app_tbl_t *entry_data;
    uint32_t hash_id;
    int i, j, blk, idx, ret;

    for (i = 0; i < entry_cnt; i++) {
    	entry_head = malloc(BYTE_ALIGN(sizeof(struct gen_type) + sizeof(ip_app_tbl_t), 16));
    	if (!entry_head) {
    	    fprintf(stderr, "Allocate dynamic memory \n");
            exit(-1);
        }
        
        entry_head->my_data = ((char *)entry_head) + sizeof(struct gen_type);
        entry_data = (ip_app_tbl_t *)entry_head->my_data;
		memset(entry_data, 0, sizeof(ip_app_tbl_t));
        if (mode == 1) {
            //dip=1.1.1.1~1.1.250.250
            blk = i/62500; idx = i%62500;
            entry_data->ip_addr = ((idx%250 + 1) << 24) | ((idx/250 + 1) << 16) | 0x0101;
            entry_data->app_cnt = i%8 + 2;
            gen_random_app(entry_data);
        } else {
            //dip=1.1.x.x
            entry_data->ip_addr = (rand()/256 << 24) | (rand()/256 << 16) | 0x0101;
            entry_data->app_cnt = rand()%1000 + 1;
            gen_random_app(entry_data);
        } 

		hash_id = rte_rss_hash(&entry_data->ip_addr, 1)%IP_APP_BUCK_SZ;
    	ret = rbtree_insert(&ip_app_bucket[hash_id], entry_head, my_cmp);
    	if (ret < 0) {
    	    if (debug_flag) printf("The 0x%x already exists \n", entry_data->ip_addr);
    	    free(entry_head);
    	}
    } 

    if (debug_flag) {
        for (i = 0; i < IP_APP_BUCK_SZ; i += 10) {
            if ( ip_app_bucket[i].count == 0) continue;
            printf("[%d]: count %u \n", i, ip_app_bucket[i].count);
        }
    }

    return VOS_OK;
}

int init_ip_app_table(int max_num)
{
    memset(ip_app_bucket, 0, sizeof(gen_table_t)*IP_APP_BUCK_SZ); 
	
	insert_demo_data(0, max_num, 0);

    return VOS_OK;
}

int main(int argc, char *argv[])
{
    int i, ret, num, debug;
	struct gen_type *entry_head;
    ip_app_tbl_t *entry_data;
    time_t t;
	uint32_t key, hash_id;
    ip_app_tbl_t demo1;
 
    if (argc < 3) {
    	fprintf(stderr, "Usage: %s <num> <debug> \n", argv[0]);
    	exit(-1);
    }
 
    num = atoi(argv[1]);
    debug = atoi(argv[2]);
    srand((unsigned)time(&t));

    init_ip_app_table(num);

	for (i = 0; i < 1000; i++) {
	    demo1.ip_addr = (i << 16) | 0x0101;
	    //printf("search 0x%x \n", demo1.ip_addr);    
		hash_id = rte_rss_hash(&demo1.ip_addr, 1)%IP_APP_BUCK_SZ;
	    entry_head = rbtree_search(&ip_app_bucket[hash_id], &demo1, my_cmp);
	    if (entry_head != NULL) {
			key = demo1.ip_addr;
			entry_data = (ip_app_tbl_t *)entry_head->my_data;
			printf("app_cnt %d \n", entry_data->app_cnt);
	    }
	}

    demo1.ip_addr = 0x01010101;
	hash_id = rte_rss_hash(&demo1.ip_addr, 1)%IP_APP_BUCK_SZ;
    ret = rbtree_delete(&ip_app_bucket[hash_id], &demo1, my_cmp);
    printf("delete 0x%x ret %d \n", demo1.ip_addr, ret);    
	
    demo1.ip_addr = key;
	hash_id = rte_rss_hash(&demo1.ip_addr, 1)%IP_APP_BUCK_SZ;
    ret = rbtree_delete(&ip_app_bucket[hash_id], &demo1, my_cmp);
    printf("delete 0x%x ret %d \n", demo1.ip_addr, ret);    
 
    printf("print_rbtree \n");
	for (i = 0; i < IP_APP_BUCK_SZ; i += 10) {
		if ( ip_app_bucket[i].count == 0) continue;
		printf("[%d]: count %u \n", i, ip_app_bucket[i].count);
	}
 
    return 0;
}


