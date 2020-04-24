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

int my_init_data[] = 
{
	0, 0, 0, 0, 0, 0, 0, 3, 0,
	0, 2, 9, 0, 8, 0, 7, 0, 0,
	6, 8, 0, 0, 0, 0, 1, 0, 4,
	0, 0, 0, 0, 4, 3, 0, 0, 0,
	0, 1, 0, 2, 0, 5, 0, 9, 0,
	0, 0, 0, 6, 9, 0, 0, 0, 0,
	7, 0, 6, 0, 0, 0, 0, 1, 5,
	0, 0, 2, 0, 1, 0, 4, 7, 0,
	0, 9, 0, 0, 0, 0, 0, 0, 0
};

int my_init_data5[] = 
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

int get_all_checked_num(NODE_INFO_ST *node_db)
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
	
	printf("node db: line(%d) checked(%d)\n", line, get_all_checked_num(node_db));
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


int try_node_db(int index)
{
    int i, j, last_bit;
	int bitmap_bak;
	
	if(index >= MATRIX_NUM) return 0;

    for(i = 0; i < MATRIX_NUM; i++) 
    {
		if (my_node_db[i].value > 0) continue;
        bitmap_bak = my_node_db[i].bitmap;
        
		for(j = 0; j < MATRIX_DEEP; j++)
		{
			if ( TEST_BITMAP(bitmap_bak, j) )
			{
				// try bit j 
				my_node_db[i].bitmap = 1 << j;
				my_node_db[i].value = j + 1;
				if ( check_node_db(my_node_db) ) return -1;
				
				return try_node_db(index + 1);
			}
		}
    }

    return 0; // done
}

int main()
{
    // get init data
	//get_init_data(my_init_data);
	show_init_data(my_init_data);

    // generate node db
	init_node_db(my_node_db, my_init_data);
	show_node_db(__LINE__, my_node_db);
  
	// try
	try_node_db(0);
    if (get_all_checked_num(my_node_db) < MATRIX_NUM)
    {
		printf("FAILED...\n");
    }
	else
	{
		show_node_db(__LINE__, my_node_db);
	}
	
	return 0;
}

