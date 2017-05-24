#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define CELL_DEEP		3
#define MATRIX_DEEP  	9
#define MATRIX_NUM  	(MATRIX_DEEP*MATRIX_DEEP)
#define FULL_BITMAP     0x1FF


#define TEST_BITMAP(bitmap, bit)   	(bitmap) & (1 << (bit))
#define CLEAR_BITMAP(bitmap, bit)   (bitmap) &= ~(1 << (bit))

int GET_BIT_NUM(int bitmap, int *last_bit)
{
	int bit_num = 0;
	int i;
	
	for(i = 0; i < MATRIX_DEEP; i++) 
    {
		if(TEST_BITMAP(bitmap, i)) 
        {
			if(NULL != last_bit) *last_bit = i;
            
			bit_num++;
		}
	}
	
	return bit_num;
}

int BITMAP_TO_STR(int bitmap, char *buffer)
{
    int i, j;
    
    if(NULL == buffer) return -1;

	for(i = 0, j = 0; i < MATRIX_DEEP; i++) {
    	if(TEST_BITMAP(bitmap, i)) {
    		buffer[j++] = '1' + i;
    	}
    }
    buffer[j] = 0;

    return 0;
}

int my_init_data1[] = 
{
	0, 6, 0, 0, 0, 3, 0, 7, 0,
	0, 0, 0, 0, 1, 5, 0, 0, 3,
	0, 7, 9, 0, 2, 0, 0, 5, 0,
	0, 0, 3, 8, 0, 2, 0, 0, 7,
	4, 0, 8, 0, 3, 0, 5, 0, 2,
	6, 0, 0, 5, 0, 1, 9, 0, 0,
	0, 5, 0, 0, 6, 0, 7, 8, 0,
	7, 0, 0, 1, 5, 0, 0, 0, 0,
	0, 3, 0, 2, 0, 0, 0, 4, 0
};

int my_init_data2[] = 
{
	6, 0, 0, 0, 4, 0, 0, 0, 5,
	0, 0, 0, 5, 0, 9, 0, 0, 0,
	0, 0, 5, 2, 0, 8, 4, 0, 0,
	0, 3, 7, 0, 0, 0, 5, 4, 0,
	5, 0, 0, 0, 0, 0, 0, 0, 9,
	0, 2, 8, 0, 0, 0, 6, 7, 0,
	0, 0, 9, 8, 0, 2, 7, 0, 0,
	0, 0, 0, 3, 0, 1, 0, 0, 0,
	2, 0, 0, 0, 7, 0, 0, 0, 3
};

int my_init_data3[] = 
{
	4, 0, 0, 0, 0, 7, 0, 0, 6,
	9, 0, 7, 3, 0, 0, 0, 0, 0,
	8, 0, 0, 2, 5, 0, 0, 0, 0,
	0, 6, 0, 5, 0, 0, 0, 0, 8,
	0, 0, 9, 0, 0, 0, 2, 0, 0,
	2, 0, 0, 0, 0, 3, 0, 1, 0,
	0, 0, 0, 0, 3, 2, 0, 0, 1,
	0, 0, 0, 0, 0, 6, 8, 0, 7,
	1, 0, 0, 4, 0, 0, 0, 0, 3
};

int my_init_data4[] = 
{
	0, 0, 8, 0, 0, 6, 2, 5, 0,
	0, 0, 0, 0, 7, 0, 0, 3, 0,
	0, 0, 0, 0, 1, 2, 9, 8, 0,
	0, 0, 5, 0, 0, 3, 0, 0, 0,
	0, 2, 0, 7, 0, 1, 0, 6, 0,
	0, 0, 0, 8, 0, 0, 1, 0, 0,
	0, 3, 6, 2, 8, 0, 0, 0, 0,
	0, 7, 0, 0, 9, 0, 0, 0, 0,
	0, 8, 2, 1, 0, 0, 4, 0, 0
};

int my_init_data[] = 
{
	0, 6, 0, 0, 0, 3, 0, 0, 0,
	0, 5, 0, 0, 0, 7, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 6, 0, 5,
	0, 0, 0, 0, 7, 0, 0, 0, 0,
	0, 0, 8, 0, 0, 0, 5, 0, 0,
	0, 0, 0, 0, 2, 0, 0, 0, 0,
	8, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 5, 0, 0, 0, 9, 0,
	0, 0, 0, 8, 0, 0, 0, 2, 0
};

typedef struct tagNODE_INFO_ST
{
	int value; // 0 means not sure
	int bitmap;
}NODE_INFO_ST;

NODE_INFO_ST my_node_db[MATRIX_NUM];

int get_init_data()
{
    int i;
	
INPUT_REP:	
	printf("input initial data: (0-null) \n");
    for(i = 0; i < MATRIX_DEEP; i++) {
        printf("line %d: ", i + 1);
        scanf("%d %d %d %d %d %d %d %d %d", 
            &my_init_data[i*MATRIX_DEEP], &my_init_data[i*MATRIX_DEEP+1], &my_init_data[i*MATRIX_DEEP+2],
            &my_init_data[i*MATRIX_DEEP+3], &my_init_data[i*MATRIX_DEEP+4], &my_init_data[i*MATRIX_DEEP+5],
            &my_init_data[i*MATRIX_DEEP+6], &my_init_data[i*MATRIX_DEEP+7], &my_init_data[i*MATRIX_DEEP+8]);
    }
	
    for(i = 0; i < MATRIX_NUM; i++) {
        if(my_init_data[i] < 0 || my_init_data[i] > MATRIX_DEEP) {
			printf("illegal data of %d, try again\n", i);
			goto INPUT_REP;
		}
    }	
	return 0;
}

int init_node_db(NODE_INFO_ST *node_db, int *init_data)
{
	int i;
	
    assert(node_db != NULL);
    assert(init_data != NULL);
	
	for(i=0; i < MATRIX_NUM; i++) {
		if (init_data[i] > MATRIX_DEEP || init_data[i] < 0) return -2;
		
		if(init_data[i] > 0) {
			node_db[i].value = init_data[i];
			node_db[i].bitmap = 1 << (init_data[i] - 1);
		} else {
			node_db[i].value = 0;
			node_db[i].bitmap = FULL_BITMAP;
		}
	}
	
	return 0;
}

int show_init_data(int *init_data)
{
	int i;
	
    assert(init_data != NULL);
	
	printf("init data: \n");
	for(i = 0; i < MATRIX_NUM; i++) {
		printf("%d ", init_data[i]);
		if((i+1)%MATRIX_DEEP == 0)printf("\n");
	}
	return 0;	
}

int get_checked_num(NODE_INFO_ST *node_db)
{
	int checked_num = 0;
	int i;
	
    assert(node_db != NULL);
    
	for(i = 0; i < MATRIX_NUM; i++) {
		if (node_db[i].value > 0) {
			checked_num++;
		}		
	}
	
	return checked_num;	
}

int get_all_bit_num(NODE_INFO_ST *node_db)
{
	int bit_num = 0;
	int i;
	
    assert(node_db != NULL);
    
	for(i = 0; i < MATRIX_NUM; i++) {
        bit_num += GET_BIT_NUM(node_db[i].bitmap, NULL);
	}
	
	return bit_num;	
}


int show_node_db(int line, NODE_INFO_ST *node_db)
{
	int i;
    char buffer[16];
	
    assert(node_db != NULL);
	
	printf("node db: line(%d) checked(%d)\n", line, get_checked_num(node_db));
	for(i = 0; i < MATRIX_NUM; i++) {
		if (node_db[i].value > 0) {
			printf("<==%d==> ", node_db[i].value);
       } else {
            BITMAP_TO_STR(node_db[i].bitmap, buffer);
			printf("%7s ", buffer);
        }
		if((i+1)%MATRIX_DEEP == 0)printf("\n");
	}
	
	return 0;
}


int load_sub_node_db(int mode, NODE_INFO_ST *node_db, int i, NODE_INFO_ST* sub_list)
{
    int j, k, row;
    int cell_x, cell_y;
    
    assert(node_db != NULL);
    assert(sub_list != NULL);

    if(mode == 1) // row(i)
    {
        for (j = i*MATRIX_DEEP, k = 0; j < i*MATRIX_DEEP + MATRIX_DEEP; j++) {
            memcpy(&sub_list[k++], &node_db[j], sizeof(NODE_INFO_ST));
        }    
    }
    else if (mode == 2) // col(i)
    {
        for (j = i, k = 0; j < MATRIX_NUM; j+= MATRIX_DEEP) {
            memcpy(&sub_list[k++], &node_db[j], sizeof(NODE_INFO_ST));
        }
    }
    else if (mode == 3) // cell(i)
    {
        cell_x = (i/CELL_DEEP)*CELL_DEEP;
        cell_y = (i%CELL_DEEP)*CELL_DEEP;
        for(row = 0, k = 0; row < CELL_DEEP; row++) {
				for (j = (cell_x + row)*MATRIX_DEEP + cell_y; j < (cell_x + row)*MATRIX_DEEP + cell_y + CELL_DEEP; j++) {
					memcpy(&sub_list[k++], &node_db[j], sizeof(NODE_INFO_ST));
				}
			}        
    }

    return 0;        
}

int restore_sub_node_db(int mode, NODE_INFO_ST *node_db, int i, NODE_INFO_ST* sub_list)
{
    int j, k, row;
    int cell_x, cell_y;
    
    assert(node_db != NULL);
    assert(sub_list != NULL);

    if(mode == 1) // row(i)
    {
        for (j = i*MATRIX_DEEP, k = 0; j < i*MATRIX_DEEP + MATRIX_DEEP; j++) {
            memcpy(&node_db[j], &sub_list[k++], sizeof(NODE_INFO_ST));
        }    
    }
    else if (mode == 2) // col(i)
    {
        for (j = i, k = 0; j < MATRIX_NUM; j+= MATRIX_DEEP) {
            memcpy(&node_db[j], &sub_list[k++], sizeof(NODE_INFO_ST));
        }
    }
    else if (mode == 3) // cell(i)
    {
        cell_x = (i/CELL_DEEP)*CELL_DEEP;
        cell_y = (i%CELL_DEEP)*CELL_DEEP;
        for(row = 0, k = 0; row < CELL_DEEP; row++) {
                for (j = (cell_x + row)*MATRIX_DEEP + cell_y; j < (cell_x + row)*MATRIX_DEEP + cell_y + CELL_DEEP; j++) {
                    memcpy(&node_db[j], &sub_list[k++], sizeof(NODE_INFO_ST));
                }
            }        
    }

    return 0;        
}

#define CHECK_BITMAP(node, last_bit)  if(GET_BIT_NUM(node.bitmap, &last_bit) == 1) node.value = last_bit + 1;

	
int process_sub_node_db_m1(NODE_INFO_ST* sub_list)
{
    int i, j, k, last_bit;
    
    assert(sub_list != NULL);
    
    // 排除已知数字的节点
    for(i = 0; i < MATRIX_DEEP; i++) {
        if (sub_list[i].value > 0) {
            for (j = 0; j < MATRIX_DEEP; j++) {
                if(j != i) {
					CLEAR_BITMAP(sub_list[j].bitmap, sub_list[i].value - 1);
					CHECK_BITMAP(sub_list[j], last_bit);
				}
            }
        }
    }

    // 如果两个节点只有2个bit且取值一致，其他节点可以排除这2个bit
    for(i = 0; i < MATRIX_DEEP; i++) {
        if (GET_BIT_NUM(sub_list[i].bitmap, NULL) == 2) {
            for (j = 0; j < MATRIX_DEEP; j++) {
                if(j != i && sub_list[j].bitmap == sub_list[i].bitmap) {
                    for (k = 0; k < MATRIX_DEEP; k++) {
                        if (k != i && k !=j && sub_list[k].value == 0) {
							//printf("process_sub_node_db_m1(%d): catch one \n", __LINE__);
                            sub_list[k].bitmap &= ~sub_list[i].bitmap;
							CHECK_BITMAP(sub_list[k], last_bit);
                        }
                    }
                }
            }
        }
    }   
    
    // 如果三个节点只有3个bit且取值一致，其他节点可以排除这3个bit
    
    return 0;
}


int process_sub_node_db_m2(NODE_INFO_ST* sub_list)
{
    int i, j, last_bit, temp_bit;
    int temp_bitmap;
    
    assert(sub_list != NULL);
    
    // 如果某个bit只存在于一个节点，则该节点可设置为checked，并清除其他节点对应的bit
    for(i = 0; i < MATRIX_DEEP; i++) {
        if (sub_list[i].value > 0) continue;

        temp_bitmap = FULL_BITMAP;
        for (j = 0; j < MATRIX_DEEP; j++) {
            if (sub_list[j].value > 0 || j == i) continue;
            temp_bitmap &= (sub_list[i].bitmap | sub_list[j].bitmap) - sub_list[j].bitmap;
        }

        if(GET_BIT_NUM(temp_bitmap, &last_bit) == 1) {
            sub_list[i].value = last_bit + 1;
            sub_list[i].bitmap = temp_bitmap;

            for (j = 0; j < MATRIX_DEEP; j++) {
                if (sub_list[j].value > 0) continue;
                if(j != i) {
					//printf("process_sub_node_db_m2(%d): catch one \n", __LINE__);
					CLEAR_BITMAP(sub_list[j].bitmap, last_bit);
					CHECK_BITMAP(sub_list[j], temp_bit);
				}
            }
        }
    }

    // 如果两个bit只存在于两个节点，则这两个节点可清除其他bit，其他节点可清除这两个bit

    return 0;
}

int scan_node_db_mode(NODE_INFO_ST *node_db, int mode)
{
	int i, ret = 0;
    NODE_INFO_ST sub_node_db[MATRIX_DEEP];
	
	assert(node_db != NULL);
	
	for(i = 0; i < MATRIX_DEEP; i++) 
    {
        ret |= load_sub_node_db(mode, node_db, i, sub_node_db);
        ret |= process_sub_node_db_m1(sub_node_db);
        ret |= process_sub_node_db_m2(sub_node_db);
        ret |= restore_sub_node_db(mode, node_db, i, sub_node_db);
        if (ret != 0) return ret;
	}

	return 0;
}

int scan_node_db_rep(NODE_INFO_ST *node_db)
{
	int checked_num;
    int bit_num;

	while(1){
		checked_num = get_checked_num(node_db);
        bit_num = get_all_bit_num(node_db);
		
		scan_node_db_mode(node_db, 1);
		scan_node_db_mode(node_db, 2);
		scan_node_db_mode(node_db, 3);

		if (checked_num == get_checked_num(node_db) && bit_num == get_all_bit_num(node_db))
			break;
	}

    return 0;
}

int check_sub_node_db(NODE_INFO_ST* sub_list)
{
    int i, j;
    
    assert(sub_list != NULL);
    
    // 检查是否存在节点取值冲突
    for(i = 0; i < MATRIX_DEEP; i++) {
        for (j = 0; j < MATRIX_DEEP; j++) {
            if(j != i && sub_list[i].value > 0 && sub_list[i].value == sub_list[j].value) return 1;
        }
    }

    return 0;
}

int check_node_db(NODE_INFO_ST *node_db)
{
    int i, ret = 0;
    NODE_INFO_ST sub_node_db[MATRIX_DEEP];
    
    assert(node_db != NULL);
    
    for(i = 0; i < MATRIX_DEEP; i++) 
    {
        load_sub_node_db(1, node_db, i, sub_node_db);
        ret = check_sub_node_db(sub_node_db);
        restore_sub_node_db(1, node_db, i, sub_node_db);
        if (ret != 0) return ret;
    }

    for(i = 0; i < MATRIX_DEEP; i++) 
    {
        load_sub_node_db(2, node_db, i, sub_node_db);
        ret = check_sub_node_db(sub_node_db);
        restore_sub_node_db(2, node_db, i, sub_node_db);
        if (ret != 0) return ret;
    }

    for(i = 0; i < MATRIX_DEEP; i++) 
    {
        load_sub_node_db(3, node_db, i, sub_node_db);
        ret = check_sub_node_db(sub_node_db);
        restore_sub_node_db(3, node_db, i, sub_node_db);
        if (ret != 0) return ret;
    }    

    return 0;
}


int try_node_db(NODE_INFO_ST *node_db)
{
    int i, j, last_bit;
    int clear_cnt = 0;
	int bitmap_bak;
    NODE_INFO_ST new_node_db[MATRIX_NUM];
    
    assert(node_db != NULL);

    for(i = 0; i < MATRIX_NUM; i++) 
    {
		if (node_db[i].value > 0) continue;
        bitmap_bak = node_db[i].bitmap;
        
		for(j = 0; j < MATRIX_DEEP; j++)
		{
			if ( TEST_BITMAP(bitmap_bak, j) )
			{
				// try bit j 
				memcpy(new_node_db, node_db, MATRIX_NUM*sizeof(NODE_INFO_ST));
				new_node_db[i].bitmap = 1 << j;
				new_node_db[i].value = j + 1;
				scan_node_db_rep(new_node_db);

				//check if conflict
				if ( check_node_db(new_node_db) ) 
				{
					printf("try_node_db_all(%d): catch %d, num %d\n", __LINE__, i, j + 1);
					CLEAR_BITMAP(node_db[i].bitmap, j);
					CHECK_BITMAP(node_db[i], last_bit);
					clear_cnt++;
				}
			}
		}
    }

    return clear_cnt++;;
}

int try_node_db_rep(NODE_INFO_ST *node_db)
{
	int checked_num;
    int bit_num;
	int clear_cnt = 0;

	while(1){
		checked_num = get_checked_num(node_db);
        bit_num = get_all_bit_num(node_db);
		
		clear_cnt += try_node_db(node_db);

		if (checked_num == get_checked_num(node_db) && bit_num == get_all_bit_num(node_db))
			break;
	}

    return clear_cnt;
}

int main()
{
    // get init data
	get_init_data(my_init_data);
	show_init_data(my_init_data);

    // generate node db
	init_node_db(my_node_db, my_init_data);
	show_node_db(__LINE__, my_node_db);

SCAN_NODE_TAG:
    // scan 
	printf("scan...\n");
    scan_node_db_rep(my_node_db);
	show_node_db(__LINE__, my_node_db);
    
	// try
    if (get_checked_num(my_node_db) < MATRIX_NUM)
    {
		printf("try...\n");
        if( try_node_db(my_node_db) > 0) goto SCAN_NODE_TAG;        
    }
	
	return 0;
}

