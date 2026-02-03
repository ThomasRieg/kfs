#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(void) {
	unsigned int *mapped = mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	printf("return from mmap: %p\n", mapped);
	int f = fork();
	printf("return from fork: %d\n", f);
	if (f == 0) {
		raise(SIGSTOP);
		printf("on child: %u\n", mapped[5]);
	} else {
		mapped[5] = 42;
		printf("memory set\n");
		usleep(1000);
		kill(f, SIGCONT);
	}
}
