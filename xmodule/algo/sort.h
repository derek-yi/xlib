
#ifndef _XLIB_SORT_H_
#define _XLIB_SORT_H_


void bubble_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*));

void cocktail_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*));

void select_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*));

void insert_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*));

void shell_sort(void *base, size_t num, size_t width, int(*compare)(void*, void*));


#endif //_XLIB_SORT_H_

