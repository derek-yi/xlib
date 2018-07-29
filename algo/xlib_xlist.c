

#include "xlib.h"

#if T_DESC("header", 1)

typedef struct xlib_entry_t
{
	int valid;
    void *entry_data;
}xlib_entry_s;

typedef struct xlib_table_t
{
	int valid;
    int type;

    int entry_size;
    int entry_cnt;
    xlib_entry_s *entry_list;
}xlib_table_s;

typedef int (*entry_cmp_func)(void *user_data, void *db_data);

#define MAX_XLIB_TABLE_SIZE     32

#define XLIB_TYPE_CHECK(type)   if(type >= MAX_XLIB_TABLE_SIZE) return XLIB_ECODE_INVALID_PARAM;

#endif


#if T_DESC("source", 1)

xlib_table_s glb_xlib_table[MAX_XLIB_TABLE_SIZE] = {0};

int xlist_init(int type, int entry_cnt, int entry_size)
{
    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid) {
        return XLIB_ECODE_HAS_INITED;
    }

    glb_xlib_table[type].entry_cnt = entry_cnt;
    glb_xlib_table[type].entry_size = entry_size;
    glb_xlib_table[type].entry_list = malloc(entry_cnt*sizeof(xlib_entry_s));
    if(glb_xlib_table[type].entry_list == NULL) {
        return XLIB_ECODE_MALLOC_FAIL;
    }
    
    memset(glb_xlib_table[type].entry_list, 0, entry_cnt*sizeof(xlib_entry_s));
    glb_xlib_table[type].valid = XLIB_STATE_TRUE;
    
    return XLIB_ECODE_OK;
}

int xlist_free(int type)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid != XLIB_STATE_TRUE) {
        return XLIB_ECODE_NOT_INITED;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if( (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE)
            && (glb_xlib_table[type].entry_list[i].entry_data != NULL) ){
            free(glb_xlib_table[type].entry_list[i].entry_data);
        }
    }

    free(glb_xlib_table[type].entry_list);
    memset(&glb_xlib_table[type], 0, sizeof(xlib_table_s));
    glb_xlib_table[type].valid = XLIB_STATE_FALSE;

    return XLIB_ECODE_OK;
}

int xlist_entry_add(int type, entry_cmp_func cmp_func, void *user_data, int *entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if(glb_xlib_table[type].valid != XLIB_STATE_TRUE) {
        return XLIB_ECODE_NOT_INITED;
    }

    if(user_data == NULL) {
        return XLIB_ECODE_INVALID_PARAM;
    }

    if( cmp_func == NULL) goto ADD_ENTRY;

//UPDATE_ENTRY:
    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                memcpy(glb_xlib_table[type].entry_list[i].entry_data, user_data, glb_xlib_table[type].entry_size);
                if(entry_index != NULL) *entry_index = i;
                return XLIB_ECODE_OK;
            }
        }
    }

ADD_ENTRY:
    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_FALSE) {
            glb_xlib_table[type].entry_list[i].entry_data = malloc(glb_xlib_table[type].entry_size);
            if(glb_xlib_table[type].entry_list[i].entry_data == NULL) {
                return XLIB_ECODE_MALLOC_FAIL;
            }

            memcpy(glb_xlib_table[type].entry_list[i].entry_data, user_data, glb_xlib_table[type].entry_size);
            glb_xlib_table[type].entry_list[i].valid = XLIB_STATE_TRUE;
            if(entry_index != NULL) *entry_index = i;
            return XLIB_ECODE_OK;
        }
    }

    return XLIB_ECODE_DB_FULL;
}

int xlist_entry_get(int type, entry_cmp_func cmp_func, void *user_data)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != XLIB_STATE_TRUE ) {
        return XLIB_ECODE_NOT_INITED;
    }

    if(user_data == NULL || cmp_func == NULL) {
        return XLIB_ECODE_INVALID_PARAM;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                memcpy(user_data, glb_xlib_table[type].entry_list[i].entry_data, glb_xlib_table[type].entry_size);
                return XLIB_ECODE_OK;
            }
        }
    }

    return XLIB_ECODE_NOT_FOUND;
}


int xlist_entry_get_index(int type, void *user_data, int entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != XLIB_STATE_TRUE ) {
        return XLIB_ECODE_NOT_INITED;
    }

    if( user_data == NULL || entry_index >= glb_xlib_table[type].entry_cnt) {
        return XLIB_ECODE_INVALID_PARAM;
    }

    i = entry_index;
    if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE) {
        memcpy(user_data, glb_xlib_table[type].entry_list[i].entry_data, glb_xlib_table[type].entry_size);
        return XLIB_ECODE_OK;
    }

    return XLIB_ECODE_NOT_FOUND;
}


int xlist_entry_delete(int type, entry_cmp_func cmp_func, void *user_data)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != XLIB_STATE_TRUE ) {
        return XLIB_ECODE_NOT_INITED;
    }

    if(user_data == NULL || cmp_func == NULL) {
        return XLIB_ECODE_INVALID_PARAM;
    }

    for(i = 0; i < glb_xlib_table[type].entry_cnt; i++) {
        if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE) {
            if(cmp_func(user_data, glb_xlib_table[type].entry_list[i].entry_data) == 0) {
                free(glb_xlib_table[type].entry_list[i].entry_data);
                glb_xlib_table[type].entry_list[i].entry_data = NULL;
                glb_xlib_table[type].entry_list[i].valid = XLIB_STATE_FALSE;
                return XLIB_ECODE_OK;
            }
        }
    }

    return XLIB_ECODE_NOT_FOUND;
}


int xlist_entry_delete_index(int type, int entry_index)
{
    int i;

    XLIB_TYPE_CHECK(type);

    if ( glb_xlib_table[type].valid != XLIB_STATE_TRUE ) {
        return XLIB_ECODE_NOT_INITED;
    }

    if( entry_index >= glb_xlib_table[type].entry_cnt) {
        return XLIB_ECODE_INVALID_PARAM;
    }

    i = entry_index;
    if (glb_xlib_table[type].entry_list[i].valid == XLIB_STATE_TRUE) {
        free(glb_xlib_table[type].entry_list[i].entry_data);
        glb_xlib_table[type].entry_list[i].entry_data = NULL;
        glb_xlib_table[type].entry_list[i].valid = XLIB_STATE_FALSE;
        return XLIB_ECODE_OK;
    }

    return XLIB_ECODE_NOT_FOUND;
}



#endif

#if T_DESC("test", DEBUG_ENABLE)

typedef struct xlib_demo_data_t
{
    int port;
    int vlan;
    int ipaddr;
}xlib_demo_data_s;

int demo_data_cmp(void *user_data, void *db_data)
{
    xlib_demo_data_s *user_entry = (xlib_demo_data_s *)user_data;
    xlib_demo_data_s *db_entry = (xlib_demo_data_s *)db_data;

    if(user_entry == NULL || db_entry == NULL) return 1;
    
    if( (user_entry->port == db_entry->port)
        && (user_entry->vlan == db_entry->vlan) )
        return 0;

    return 1;
}

#ifdef BUILD_XLIB_SO
int xlib_xlist_test()
#else
int main()
#endif
{  
    int ret;
    int entry_index;
    xlib_demo_data_s demo_data;

//table test
    ret = xlist_init(1, 100, sizeof(xlib_demo_data_s)); //init t1
    XLIB_RET_CHECK("xlist_init", ret, XLIB_ECODE_OK);

    ret = xlist_init(1, 200, sizeof(xlib_demo_data_s)); //re-init t1
    XLIB_RET_CHECK("xlist_init", ret, XLIB_ECODE_HAS_INITED);

    ret = xlist_init(2, 100, sizeof(xlib_demo_data_s)); //init t2
    XLIB_RET_CHECK("xlist_init", ret, XLIB_ECODE_OK);

    ret = xlist_free(1); //free t1
    XLIB_RET_CHECK("xlist_free", ret, XLIB_ECODE_OK);

    ret = xlist_init(1, 200, sizeof(xlib_demo_data_s)); //re-init t1
    XLIB_RET_CHECK("xlist_init", ret, XLIB_ECODE_OK);

//entry test
    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 100;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //add e1 to t1
    XLIB_RET_CHECK("xlist_entry_add", ret, XLIB_ECODE_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 200;
    demo_data.vlan = 200;
    demo_data.ipaddr = 200;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //add e2 to t1
    XLIB_RET_CHECK("xlist_entry_add", ret, XLIB_ECODE_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 200;
    ret = xlist_entry_add(1, demo_data_cmp, &demo_data, &entry_index); //update e1
    XLIB_RET_CHECK("xlist_entry_add", ret, XLIB_ECODE_OK);
    //printf("\r\n entry=%d valid=%d", entry_index, glb_xlib_table[1].entry_list[entry_index].valid);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 0;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get e1
    XLIB_RET_CHECK("xlist_entry_get", ret, XLIB_ECODE_OK);
    XLIB_RET_CHECK("xlist_entry_get", demo_data.ipaddr, 200);

    demo_data.port = 0;
    demo_data.vlan = 0;
    demo_data.ipaddr = 0;
    ret = xlist_entry_get_index(1, &demo_data, entry_index); //get e1
    XLIB_RET_CHECK("xlist_entry_get_index", ret, XLIB_ECODE_OK);
    XLIB_RET_CHECK("xlist_entry_get_index", demo_data.port, 100);
    XLIB_RET_CHECK("xlist_entry_get_index", demo_data.ipaddr, 200);

    demo_data.port = 100;
    demo_data.vlan = 100;
    demo_data.ipaddr = 0;
    ret = xlist_entry_delete(1, demo_data_cmp, &demo_data); //delete e1
    XLIB_RET_CHECK("xlist_entry_delete", ret, XLIB_ECODE_OK);

    demo_data.port = 100;
    demo_data.vlan = 100;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get e1
    XLIB_RET_CHECK("xlist_entry_get", ret, XLIB_ECODE_NOT_FOUND);

    demo_data.port = 500;
    demo_data.vlan = 500;
    ret = xlist_entry_get(1, demo_data_cmp, &demo_data); //get unknown
    XLIB_RET_CHECK("xlist_entry_get", ret, XLIB_ECODE_NOT_FOUND);

    demo_data.port = 500;
    demo_data.vlan = 500;
    ret = xlist_entry_delete(1, demo_data_cmp, &demo_data); //delete unknown
    XLIB_RET_CHECK("xlist_entry_delete", ret, XLIB_ECODE_NOT_FOUND);

//clean    
    ret = xlist_free(1); //free t1
    XLIB_RET_CHECK("xlist_free", ret, XLIB_ECODE_OK);

    ret = xlist_free(2); //free t2
    XLIB_RET_CHECK("xlist_free", ret, XLIB_ECODE_OK);

    return XLIB_ECODE_OK;
}

#endif




