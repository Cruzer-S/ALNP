#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <threads.h>

char argv0[64];

void sh_sigusr(int signum)
{
	printf("\t%s[%d]: Signal Handler(%s)\n",
			argv0, getpid(), signum == SIGUSR1 ? "USR1" : "USR2");
}

void sh_sigterm(int signum)
{
	printf("\t%s[%d]: Signal Handler(SIGTERM)\n", argv0, getpid());
	exit(signum);
}

int main(int argc, char *argv[])
{
	strncpy(argv0, argv[0], strlen(argv[0]));
	struct sigaction sa_usr1, sa_term;

	memset(&sa_usr1, 0, sizeof(struct sigaction));
	sa_usr1.sa_handler = sh_sigusr;
	sigfillset(&sa_usr1.sa_mask);

	memset(&sa_term, 0, sizeof(struct sigaction));
	sa_term.sa_handler = sh_sigterm;

	sigaction(SIGUSR1, &sa_usr1, NULL);
	sigaction(SIGUSR2, &sa_usr1, NULL);
	sigaction(SIGTERM, &sa_term, NULL);

	dprintf(STDOUT_FILENO, "%s[%d]: Start\n", argv0, getpid());
	for (;;) {
		pause();
	}

	dprintf(STDOUT_FILENO, "%s[%d]: Exit\n", argv0, getpid());

	fflush(NULL);

	return 0;
}
