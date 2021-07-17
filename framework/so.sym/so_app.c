
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h> /* dlopen, dlsym, dlclose */

void test1();
void test2();
void test3();

int main(int argc, char **argv)
{
    test1();
    test2();
    test3();

    /* below only for testing dynamic linking loader */
    void *handle;
    void (*test)();
    char *error;

    handle = dlopen ("./libdemo.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        exit(1);
    }

    test = dlsym(handle, "test2");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }

    (*test)();
    dlclose(handle);

    return 0;
}
