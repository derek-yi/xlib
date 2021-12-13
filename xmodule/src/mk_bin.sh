## syscfg
gcc -o syscfg.bin syscfg.c cJSON.c -I../include -lpthread

## tiny_cli
gcc -o tiny_cli.bin tiny_cli.c -I../include -lpthread

## xlog
gcc -o xlog.bin xlog.c vos.c -I../include -lpthread -lrt

## syscfg
gcc -o syscfg.bin syscfg.c cJSON.c -I../include -lpthread

## xmodule
gcc -DMAKE_APP -DMAKE_XLIB -o xmodule.bin  xmodule.c xmsg.c xlog.c tiny_cli.c syscfg.c vos.c cJSON.c -I../include -lpthread -lrt


