#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BYTES 10000000
#define AT_EMPTY_PATH		0x1000	/* Allow empty relative pathname */

int main(void)
{
	int fd = open("/dev/tty1", O_RDWR | O_NONBLOCK);
	dup(fd);
	dup(fd);
	chdir("/etc");

	printf("\t\t//// OPENING `hostname`\n");

	FILE *fp = fopen("hostname", "rb");

	if (fp) {
		struct stat stat;
		fstatat(fileno(fp), "", &stat, AT_EMPTY_PATH);
		printf("dev %llu inode %lu mode %u\n", stat.st_dev, stat.st_ino, stat.st_mode);

		char line[4096];
		fgets(line, 4096, fp);
		printf("%s", line);

		fclose(fp);
	}
	else
		perror("fopen");
	errno = 0;
	DIR *dir = opendir(".");
	if (dir) {
		struct dirent *dirent;
		errno = 0;
		while ((dirent = readdir(dir))) {
			printf("%lu %ld %u %u %s\n", dirent->d_ino, dirent->d_off, dirent->d_reclen, dirent->d_type, dirent->d_name);
		}
		perror("readdir");
		closedir(dir);
	} else
		perror("opendir");
	unsigned char *p = malloc(BYTES);
	for (unsigned int i = 0; i < BYTES; i++)
		p[i] = 42;
	free(p);
	write(1, "hi!\n", 4);
	for (unsigned int i = 0; i < 10; i++)
	{
		printf("%u\n", i);
	}
}
