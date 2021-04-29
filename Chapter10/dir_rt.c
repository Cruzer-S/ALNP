#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/signal.h>
#include <fcntl.h>

void chk_rt(int sig, siginfo_t *siginfo_rt, void *data);

int main(int argc, char *argv[])
{
	int fd_dir;
	struct sigaction sa_rt;

	if (argc != 2) {
		printf("usage: %s <dir>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sa_rt.sa_sigaction = chk_rt;
	sigemptyset(&sa_rt.sa_mask);
	sa_rt.sa_flags = SA_SIGINFO;

	sigaction(SIGRTMIN, &sa_rt, NULL);
	fd_dir = open(argv[1], O_RDONLY);
	
	fcntl(fd_dir, F_SETSIG, SIGRTMIN);
	fcntl(fd_dir, F_NOTIFY, DN_ACCESS | DN_MODIFY | DN_MULTISHOT);

	while (true)
		pause();

	return 0;
}

void chk_rt(int sig, siginfo_t *siginfo_rt, void *data)
{
	printf("[SIGRT] si->si_band (%lx)\n",
			siginfo_rt->si_band);
}
