#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
	int sv[2];
	int socketpairr = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
	printf("socketpair returned %d + two sockets %d and %d\n", socketpairr, sv[0], sv[1]);
	if (socketpairr < 0)
		perror("socketpair");

	char buf[6] = "hello";
	int writer = write(sv[0], buf, 6);
	printf("write returned %d\n", writer);
	if (writer < 0)
		perror("writer");

	char read_buf[6];
	int readr = read(sv[1], read_buf, 6);
	printf("read returned %d: %s\n", readr, read_buf);
	if (readr < 0)
		perror("readr");
}
