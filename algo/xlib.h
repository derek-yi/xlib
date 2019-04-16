
#ifndef _XXX_LIB_H_
#define _XXX_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define XLIB_TRUE           1
#define XLIB_FALSE          0
#define XLIB_OK             0
#define XLIB_ERROR          1

#ifndef bool
#define bool    int
#endif

typedef unsigned int u32;
typedef int s32;
typedef signed long long s64;
typedef unsigned long long u64;

#ifndef true
#define true    1
#define false   0
#endif

#ifndef DEBUG_ENABLE
#define DEBUG_ENABLE        1
#endif

#ifndef T_DESC
#define T_DESC(x, y)        (y)
#endif

#define XLIB_UT_CHECK(desc, wanted, exp)  \
    if(exp != wanted) printf("\r\n %d: %s FAILED! \r\n", __LINE__, desc); \
    else printf("\r\n %d: %s PASS! \r\n", __LINE__, desc);

#define XLIB_RET_CHECK(desc, ret, wanted)  \
    if(ret != wanted) printf("\r\n %d: %s FAILED! ret=0x%x\r\n", __LINE__, desc, ret); 


#endif


