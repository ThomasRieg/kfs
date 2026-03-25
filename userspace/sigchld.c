#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld(int signo) {
	printf("sigchld received (%d)\n", signo);
	int status;
	pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
	if (pid > 0) {
		printf("pid %u\n", signo);
		if (WIFEXITED(status))
			printf("exited with status %u\n", WEXITSTATUS(status));
		else if (WIFSTOPPED(status))
			printf("stopped by sig %u\n", WSTOPSIG(status));
		else if (WIFSIGNALED(status))
			printf("terminated by sig %u\n", WTERMSIG(status));
		else
			printf("unknown stat change\n", WSTOPSIG(status));
	} else
		printf("could not get child status change\n");
}

int main(void) {
	signal(SIGCHLD, sigchld);
	pid_t pid = syscall(2);
	if (pid == 0) {
		printf("[child entering pause]\n");
		pause();
		printf("[child leaving pause]\n");
	} else if (pid > 0) {
		sleep(1);
		kill(pid, SIGSTOP);
		sleep(1);
		kill(pid, SIGKILL);
		sleep(1);
		// don't wait for child to test init adoption & reaping
	}
}
