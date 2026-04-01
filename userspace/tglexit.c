#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

void *thread_start(void *p) {
	sleep(2);
	printf("[thread] still alive!\n");
	return 0;
}

int main(void) {
	pthread_t tid;
	int s = pthread_create(&tid, NULL, &thread_start, NULL);
	printf("[thread leader] exiting\n");
	syscall(1, 0);
}

