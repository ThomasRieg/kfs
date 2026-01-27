#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BYTES 10000000

int main(void) {
	int fd = open("/dev/tty1", O_RDWR|O_NONBLOCK);
	dup(fd);
	dup(fd);
	FILE *fp = fopen("/etc/hostname", "rb");

	if (fp) {
		unsigned char line[4096];
		fgets(line, 4096, fp);
		printf("%s\n", line);

		fclose(fp);
	} else
		perror("fopen");
	unsigned char *p = malloc(BYTES);
	for (unsigned int i = 0; i < BYTES; i++)
		p[i] = 42;
	free(p);
	write(1, "hi!\n", 4);
	for (unsigned int i = 0; i < 10; i++) {
		printf("%u\n", i);
	}
}
