#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	malloc(10000000);
	write(1, "hi!\n", 4);
	for (unsigned int i = 0; i < 10; i++) {
		printf("%u\n", i);
	}
}
