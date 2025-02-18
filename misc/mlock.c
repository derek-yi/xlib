#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

const int alloc_size = 20 * 1024 * 1024;//20M

int main(int argc, char **argv)
{
	char *memory;
	int lock = 0;
	
	if (argc > 1) lock = atoi(argv[1]);
	
	if (lock == 1) {
		if(mlockall(MCL_FUTURE) == -1) {
			perror("mlock");
			return (-1);
		}
	}

#if 0
	memory = (char *)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (memory == MAP_FAILED) return -1;
#else
	memory = malloc(alloc_size);
	if (memory == NULL) return -1;
#endif

	if (lock == 2) {
		if(mlockall(MCL_CURRENT) == -1) {
			perror("mlock");
			return (-1);
		}
	}
	
#if 0 //make dirty
	size_t i;
	size_t page_size = getpagesize();
	for(i=0;i<alloc_size;i+=page_size) {
			//printf("i=%zd\n",i);
			memory[i] = 0;
	}
#endif

	printf("ready to free \n");
	getchar();
#if 0
	munmap(memory, alloc_size);
#else
	free(memory);
#endif
	
	printf("ready to exit \n");
	getchar();
	return 0;
}

