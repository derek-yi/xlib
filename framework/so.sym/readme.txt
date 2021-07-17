
## make lib
gcc -fPIC -c so_lib.c   
gcc -shared -o libdemo.so so_lib.o

## make app
gcc so_app.c -ldl -L./ -ldemo
  
## run app
export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH && ./a.out 

