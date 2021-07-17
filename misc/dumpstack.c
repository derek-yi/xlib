
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

//need -rdynamic 
void user_dump_stack(void)
{
   int j, nptrs;
   void *buffer[1024];
   char **strings;
 
   nptrs = backtrace(buffer, 32);
   printf("backtrace() returned %d addresses\n", nptrs);
 
   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	  would produce similar output to the following: */
   strings = backtrace_symbols(buffer, nptrs);
   if (strings == NULL) {
	   perror("backtrace_symbols");
	   return ;
   }
 
   for (j = 0; j < nptrs; j++)
	   printf("%s\n", strings[j]);
 
   free(strings);
}

int func_b(int param)
{
    user_dump_stack();
    return 100;
}

int func_a(int param)
{
    param += 1;
    return func_b(param);
}

int main()
{
    int param = 100;
    
    return func_a(param);
}
