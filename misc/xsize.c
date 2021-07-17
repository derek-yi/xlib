#include <stdio.h>
#include <unistd.h>

int main()
{
	printf("char=%ld short=%ld int=%ld \n", sizeof(char), sizeof(short), sizeof(int));
	printf("ulong=%ld ptr=%ld ll=%ld \n", sizeof(unsigned long), sizeof(char *), sizeof(long long));
	printf("float=%ld double=%ld \n", sizeof(float), sizeof(double));
	
	return 0;
}