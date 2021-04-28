#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

void sa_handler_chld(int signum);

int main(void)
{
	struct sigaction sa_chld;
	int ret;

	memset(&sa_chld, 0x00, sizeof(struct sigaction));
	sa_chld.sa_handler = sa_handler_chld;
	sigfillset(&sa_chld.sa_mask);
	sa_chld.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa_chld, NULL);

	pr_out("[MAIN] SIGNAL handler installed");

	switch ((ret = fork())) {
	case 0:
		pause();
		break;

	case -1:
		break;

	default:
		pr_out("- child pid = %d", ret);
		break;
	}

	while (true) {
		pause();
		pr_out("[MAIN] Receive SIGNAL");
	}

	return 0;
}

void sa_handler_chld(int signum)
{
	pid_t pid_child;
	int status;

	pr_out("[SIGNAL] Receive SIGCHLD signal");

	while (true)
	{
		if ((pid_child = waitpid(-1, &status, WNOHANG)) > 0) {
			pr_out("\t- Exit child PID(%d)", pid_child);
		} else {
			break;
		}
	}
}
