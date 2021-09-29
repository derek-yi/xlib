
## xlog
gcc -I../include -o xlog.bin xlog.c vos.c -lpthread -lrt

## syscfg
gcc -I../include -o syscfg.bin syscfg.c cJSON.c vos.c -lpthread -lrt


