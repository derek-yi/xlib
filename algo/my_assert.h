
#ifndef _MY_ASSERT_H_
#define _MY_ASSERT_H_


#define MY_ASSERT(x)    do { if (!(x)) printf("%s:%d: assert failed\r\n", __FILE__, __LINE__); } while(0)

#define XLIB_UT_CHECK(desc, wanted, exp)  \
    if(exp != wanted) printf("\r\n %d: %s FAILED! \r\n", __LINE__, desc); \
    else printf("\r\n %d: %s PASS! \r\n", __LINE__, desc);
    
#endif
