#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <execinfo.h>
#include <signal.h>

//kernel: dump_stack()

void dump(int signo)
{
    void *buffer[30] = {0};
    size_t size;
    char **strings = NULL;
    size_t i = 0;

    size = backtrace(buffer, 30);
    fprintf(stdout, "Obtained %zd stack frames.nm\n", size);
    strings = backtrace_symbols(buffer, size);
    if (strings == NULL)
    {
        perror("backtrace_symbols.");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < size; i++)
    {
        fprintf(stdout, "%s\n", strings[i]);
    }
    free(strings);
    strings = NULL;
    exit(0);
}

void func_c()
{
    *((volatile char *)0x0) = 0x9999;
}

void func_b()
{
    func_c();
}

void func_a()
{
    func_b();
}

int main(int argc, const char *argv[])
{
    if (signal(SIGSEGV, dump) == SIG_ERR)
        perror("can't catch SIGSEGV");
    
    func_a();
    return 0;
}

#if xxx

//����
gcc -g -rdynamic backtrace.c -o test

./test
Obtained6stackframes.nm
./backstrace_debug(dump+0x45)[0x80487c9]
[0x468400]
./backstrace_debug(func_b+0x8)[0x804888c]
./backstrace_debug(func_a+0x8)[0x8048896]
./backstrace_debug(main+0x33)[0x80488cb]
/lib/i386-linux-gnu/libc.so.6(__libc_start_main+0xf3)[0x129113]

//���ţ�
objdump -d test > test.s

//��test.s������804888c���£�
8048884 <func_b>:
8048884: 55          push %ebp
8048885: 89 e5      mov %esp, %ebp
8048887: e8 eb ff ff ff      call 8048877 <func_c>
804888c: 5d            pop %ebp
804888d: c3            ret

//����80488cʱ���ã�call 8048877��C������ĵ�ַ����Ȼ��û��ֱ�Ӷ�λ��C������ͨ�������룬 
//���������Ƴ���C�����������ˣ�popָ��ᵼ�¶δ���ģ���
//����Ҳ����ͨ��addr2line���鿴
addr2line 0x804888c -e backstrace_debug -f 


#endif

