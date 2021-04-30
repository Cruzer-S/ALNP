#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/signal.h>
#include <sys/signalfd.h>

#define MAX_POOL 3

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (false)

int main(int argc, char *argv[])
{
	sigset_t mask;
	int sfd;
	struct signalfd_siginfo fdsi;
	ssize_t s;
	
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		handle_error("sigprocmask");

	sfd = signalfd(-1, &mask, 0);
	if (sfd == -1)
		handle_error("signalfd");

	while (true)
	{
		s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
		if (s != sizeof(struct signalfd_siginfo))
			handle_error("read");

		if (fdsi.ssi_signo == SIGINT) {
			printf("Got SIGINT: %d/%d\n", fdsi.ssi_code, fdsi.ssi_pid);
		} else if (fdsi.ssi_signo == SIGQUIT) {
			printf("Got SIGQUIT\n");
			exit(EXIT_FAILURE);
		} else {
			printf("Read unexpected signal\n");
		}
	}

	return 0;
}
