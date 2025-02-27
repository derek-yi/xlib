#include <stdio.h>
#include <string.h>


int main()
{
    int ret, dev_valid, ip_valid, gw_valid;
    char dev_name[32];
    char usb_ip[32];
    char *fmt_str = "dev_name=usb0 dev_valid=1 ip_valid=1 gw_valid=1 usb_ip=10.195.150.98";

    ret = sscanf(fmt_str, "dev_name=%s dev_valid=%d ip_valid=%d gw_valid=%d usb_ip=%s",
          dev_name, &dev_valid, &ip_valid, &gw_valid, usb_ip);

    printf("ret = %d \n", ret);
    if (ret > 0) {
        printf("dev_name=%s dev_valid=%d ip_valid=%d gw_valid=%d usb_ip=%s \n",
              dev_name, dev_valid, ip_valid, gw_valid, usb_ip);
    }
    
    return 0;      
}

