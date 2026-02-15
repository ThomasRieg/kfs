#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <signal.h>

#define BYTES 10000000
#define AT_EMPTY_PATH 0x1000 /* Allow empty relative pathname */

int fdprintf(int fd, size_t bufmax, const char *fmt, ...)
{
	char *buffer;
	int n;
	va_list ap;

	buffer = (char *)malloc(bufmax);
	if (!buffer)
		return 0;

	va_start(ap, fmt);
	n = vsnprintf(buffer, bufmax, fmt, ap);
	va_end(ap);

	write(fd, buffer, n);
	free(buffer);
	return n;
}

static volatile __sig_atomic_t got_sigchld = 0;

static void on_sigchld(int sig)
{
    (void)sig;
    got_sigchld = 1;

    // Avoid printf in signal handlers in real POSIX; write is async-signal-safe.
    const char msg[] = "[handler] SIGCHLD received\n";
    write(1, msg, sizeof(msg) - 1);
}

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
	printf("sizeof(struct statx) = %u\n", sizeof(struct statx));
	printf("sizeof(struct statx_timestamp) = %u\n", sizeof(struct statx_timestamp));

	printf("\t\t//// CHANGING CWD TO `/etc`\n");
	chdir("/etc");
	char buf[4096];
	if (getcwd(buf, sizeof(buf)))
		printf("cwd: %s\n", buf);
	else
		perror("getcwd");

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
	char *shared = mmap(NULL, 4096 * 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (shared == MAP_FAILED)
	{
		perror("mmap");
		return (-1);
	}
	struct sigaction sa;
    sa.sa_handler = on_sigchld;
    sa.sa_flags = 0;
    memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));
    if (sigaction(SIGCHLD, &sa, NULL) != 0)
    {
        perror("sigaction(SIGCHLD)");
        return 1;
    }
	int forkr = syscall(2);
	if (forkr == 0) {
		char *const argv[] = {"/bin/sh", 0};
		char *const envp[] = {0};
		execve("/bin/sh", argv, envp);
	} else {
		uint32_t status;
		uint32_t ret = 0;
		//kill(forkr, 6);
		ret = syscall(114, forkr, &status, 0, 0);
		printf("shell pid %i exited with status %u\n", ret, status);
		return (status);
	}

	memset(shared, 69, 4096 * 4);
	u_int32_t skibidi = 67;
	int pipe_fd[2];
	if (pipe(pipe_fd))
	{
		perror("pipe");
		return (-1);
	}
	int pid = syscall(2);
	printf("return from fork %d\n", pid);
	if (pid == -1)
	{
		perror("fork");
		return (-1);
	}
	else if (pid == 0)
	{
		int childpid = syscall(2);
		if (childpid == 0)
		{
			write(pipe_fd[1], "coucou from grandchild\n", 23);
			close(pipe_fd[1]);
			printf("grandchild exiting\n");
			//while (1) ;
			return(0);
		}
		printf("hello from pid %u (child)\n", getpid());
		skibidi = 0;
		write(pipe_fd[1], "coucou from pipe", 16);
		//for (u_int32_t i = 0; i < 10000; i++)
		//	fdprintf(pipe_fd[1], 4096,  "%u", i);
		printf("child modified skybidi to %u in child\n", skibidi);
		munmap(shared + 4096, 4096);
		shared[0] = 42;
		shared[4096 * 2] = 42;
		printf("child modified shared[0] to %u in child\n", shared[0]);
		/*p = malloc(BYTES * 200);
		for (unsigned int i = 0; i < BYTES * 200; i++)
			p[i] = 42;
		free(p);*/
		close(pipe_fd[1]);
		syscall(114, childpid, 0, 0, 0);
		return (0);
	}
	else
	{
		close(pipe_fd[1]);
		printf("hello from pid %u (parent)\n", getpid());
		char buff[8093];
		int ret = read(pipe_fd[0], buff, 8092);
		while (ret > 0)
		{
			buff[ret] = 0;
			printf("message from pipe: %s ret: %i\n", buff, ret);
			ret = read(pipe_fd[0], buff, 8092);
		}
		if (ret < 0)
		{
			perror("read");
		}
		printf("waiting for child to exit\n");
		syscall(114, pid, 0, 0, 0);
		printf("skibidy still %u in parent\n", skibidi);
		printf("shared[0] changed to %u in parent, can still access shared[4097] : %u\n", shared[0], shared[4097]);
		/*p = malloc(BYTES * 200); // test that the child has been cleanup correctly (if this worked in child but not in parentm it means child cleanup didn't happen after exit)
		for (unsigned int i = 0; i < BYTES * 200; i++)
			p[i] = 42;*/
		return (0);
	}
}
