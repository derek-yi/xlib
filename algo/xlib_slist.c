



#include "xlib.h"

#if T_DESC("source", 1)

typedef struct tiny_table_tag
{
    int     table_valid;
	int     entry_size;
    int     entry_cnt;
    void    *entry_list;
    
}tiny_table_st;

#define TINY_TABLE_CNT      32
tiny_table_st  global_tiny_table[TINY_TABLE_CNT];

int tiny_table_init(int type, int entry_size, int entry_cnt)
{
    void *list_buff;
    
    global_tiny_table[type].entry_size = entry_size;
    global_tiny_table[type].entry_cnt = entry_cnt;
    global_tiny_table[type].table_valid = 1;
    list_buff = malloc(entry_cnt*entry_size);

    global_tiny_table[type].entry_list = list_buff; 
    return 0;
}

int tiny_table_add_entry(int type, void *entry_data)
{
}

#endif

#if T_DESC("test", DEBUG_ENABLE)



#ifdef BUILD_XLIB_SO
int xlib_slist_test()
#else
int main()
#endif
{  


}

#endif

