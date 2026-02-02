#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BYTES 10000000
#define AT_EMPTY_PATH 0x1000 /* Allow empty relative pathname */

int main(void)
{
	int fd = open("/dev/tty1", O_RDWR | O_NONBLOCK);
	dup(fd);
	dup(fd);
	printf("\t\t//// TTY1 opened and duplicated twice so fds 0, 1, 2 all point to terminal\n");
	printf("sizeof(long long unsigned int) = %u\n", sizeof(long long unsigned int));
	printf("sizeof(long unsigned int) = %u\n", sizeof(long unsigned int));
	printf("sizeof(unsigned int) = %u\n", sizeof(unsigned int));
	printf("sizeof(struct stat) = %u\n", sizeof(struct stat));

	printf("\t\t//// CHANGING CWD TO `/etc`\n");
	chdir("/etc");

	printf("\t\t//// OPENING `hostname`\n");

	FILE *fp = fopen("hostname", "rb");

	if (fp)
	{
		printf("\t\t//// FSTATAT `hostname`\n");
		struct stat stat;
		if (fstatat(fileno(fp), "", &stat, AT_EMPTY_PATH) == -1)
			perror("fstatat");
		else
			printf("dev %llu inode %lu mode %u size %lu\n", stat.st_dev, stat.st_ino, stat.st_mode, stat.st_size);

		printf("\t\t//// READING LINE IN `hostname`\n");
		char line[4096];
		fgets(line, 4096, fp);
		printf("%s", line);

		fclose(fp);
	}
	else
		perror("fopen");
	printf("\t\t//// READING ENTRIES IN CURRENT DIRECTORY\n");
	errno = 0;
	DIR *dir = opendir(".");
	if (dir)
	{
		struct dirent *dirent;
		errno = 0;
		while ((dirent = readdir(dir)))
		{
			printf("ino=%lu off=%ld reclen=%u type=%u name=%s\n", dirent->d_ino, dirent->d_off, dirent->d_reclen, dirent->d_type, dirent->d_name);
		}
		perror("readdir");
		closedir(dir);
	}
	else
		perror("opendir");
	printf("\t\t//// ALLOCATION TEST\n");
	unsigned char *p = malloc(BYTES);
	for (unsigned int i = 0; i < BYTES; i++)
		p[i] = 42;
	free(p);
#define GOODBYE "\t\t//// GOODBYE\n"
	write(1, GOODBYE, sizeof(GOODBYE) - 1);
	u_int32_t skibidi = 67;
	int pid = syscall(2);
	printf("return from fork %d\n", pid);
	if (pid == -1)
	{
		perror("fork");
	}
	else if (pid == 0)
	{
		printf("hello from pid %u (child)\n", getpid());
		skibidi = 0;
		printf("child modified skybidi to %u in child\n", skibidi);
		exit(0);
	}
	else
	{
		printf("hello from pid %u (parent)\n", getpid());
		printf("waiting for child to exit\n");
		syscall(114, pid, 0, 0, 0);
		printf("skibidy still %u in parent\n", skibidi);
		while (1)
			;
	}
}
