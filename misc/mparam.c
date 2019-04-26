#include <stdlib.h>
#include <stdio.h>

uint32 mparam_print(uint32 param_num, ...)
{
va_list arg_ptr; 
uint32 i; 
uint32 param[10];

va_start(arg_ptr, param_num); 
for(i = 0; i < param_num && i < 10; i++) {
param[i] = va_arg(arg_ptr, int);
}
va_end(arg_ptr);

printf("%d: ", param_num);
for(i = 0; i < param_num && i < 10; i++) {
printf("%d ", param[i]);
}
printf("\r\n");

return VOS_OK;
}

 

uint32 main()
{
//mparam_print();
mparam_print(0);

mparam_print(1);
mparam_print(1, 10);
mparam_print(1, 10, 20);

mparam_print(2, 10);
mparam_print(2, 10, 20);
mparam_print(2, 10, 20, 30);

return VOS_OK;
}
