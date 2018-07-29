
#ifndef _XXX_LIB_H_
#define _XXX_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define XLIB_STATE_TRUE                 1
#define XLIB_STATE_FALSE                0

#define XLIB_ECODE_OK                   0
#define XLIB_ECODE_ERROR                1
#define XLIB_ECODE_INVALID_PARAM        2
#define XLIB_ECODE_NOT_INITED           3
#define XLIB_ECODE_HAS_INITED           4
#define XLIB_ECODE_MALLOC_FAIL          5
#define XLIB_ECODE_NOT_FOUND            7
#define XLIB_ECODE_HAS_EXIST            8
#define XLIB_ECODE_DB_FULL              9


#ifndef DEBUG_ENABLE
#define DEBUG_ENABLE            1
#endif

#ifndef T_DESC
#define T_DESC(x, y)            (y)
#endif


#define XLIB_UT_CHECK(desc, wanted, exp)  \
    if(exp != wanted) printf("\r\n %d: %s FAILED! \r\n", __LINE__, desc); \
    else printf("\r\n %d: %s PASS! \r\n", __LINE__, desc);

#define XLIB_RET_CHECK(desc, ret, wanted)  \
    if(ret != wanted) printf("\r\n %d: %s FAILED! ret=0x%x(%d)\r\n", __LINE__, desc, ret, ret); 


#endif


