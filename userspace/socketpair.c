#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
	int sv[2];
	int socketpairr = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	printf("socketpair returned %d + two sockets %d and %d\n", socketpairr, sv[0], sv[1]);

	char buf[6] = "hello";
	int writer = write(sv[0], buf, 6);
	printf("write returned %d\n", writer);

	int readr = read(sv[1], buf, 6);
	printf("read returned %d: %s\n", readr, buf);
}
