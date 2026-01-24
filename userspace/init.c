#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	malloc(10000000);
	write(1, "hi!\n", 4);
	for (unsigned short i = 0; i < 4; i++)
		printf("%u\n", i);
}
