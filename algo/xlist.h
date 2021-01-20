
#ifndef _XLIB_XLIST_H_
#define _XLIB_XLIST_H_


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

int xlist_init(int type, int entry_cnt, int entry_size);

int xlist_free(int type);

int xlist_entry_add(int type, entry_cmp_func cmp_func, void *user_data, int *entry_index);

int xlist_entry_get(int type, entry_cmp_func cmp_func, void *user_data);

int xlist_entry_get_index(int type, void *user_data, int entry_index);

int xlist_entry_delete(int type, entry_cmp_func cmp_func, void *user_data);

int xlist_entry_delete_index(int type, int entry_index);

#endif

