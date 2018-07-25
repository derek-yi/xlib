
#ifndef _XXX_LIB_H_
#define _XXX_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


#ifndef DEBUG_ENABLE
#define DEBUG_ENABLE    1
#endif

#ifndef T_DESC
#define T_DESC(x, y)   (y)
#endif


#define XLIB_UT_CHECK(desc, x, exp)  \
    if(x != exp) printf("\r\n <%s><%d>: case %s FAILED! \r\n", __FILE__, __LINE__, desc); \
    else printf("\r\n <%s><%d>: case %s PASS! \r\n", __FILE__, __LINE__, desc);


#endif


