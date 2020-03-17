
#ifndef _XXX_LIB_H_
#define _XXX_LIB_H_

#include <stdio.h>
#include <stdlib.h>

#ifndef bool
#define bool                int
#endif

#define XLIB_OK             0
#define XLIB_ERROR          1

#ifndef TRUE
#define TRUE                1
#endif
#ifndef FALSE
#define FALSE               0
#endif

#ifndef T_DESC
#define T_DESC(x, y)        (y)
#endif


#define XLIB_UT_CHECK(desc, wanted, exp)  \
    if(exp != wanted) printf("\r\n %d: %s FAILED! \r\n", __LINE__, desc); \
    else printf("\r\n %d: %s PASS! \r\n", __LINE__, desc);

#define XLIB_UT_CHECK2(desc, ret, wanted)  \
    if(ret != wanted) printf("\r\n %d: %s FAILED! ret=0x%x\r\n", __LINE__, desc, ret); 


#endif


