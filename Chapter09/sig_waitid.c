#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>


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
	int ret;
	struct sigaction sa_chld;

	memset(&sa_chld, 0x00, sizeof(struct sigaction));
	sa_chld.sa_handler = sa_handler_chld;
	sigfillset(&sa_chld.sa_mask);

	sigaction(SIGCHLD, &sa_chld, NULL);
	
	pr_out("[MAIN] signal handler installed (pid %d)", getpid());

	switch ((ret = fork()))
	{
	case 0:
		pause();
		exit(EXIT_SUCCESS);
		break;

	case -1:
		break;

	default:
		pr_out("- child pid %d", ret);
		break;
	}

	while (true)
	{
		pause();
		pr_out("[MAIN] Receive SIGNAL...");
	}

	return 0;
}

void sa_handler_chld(int signum)
{
	int optflags;
	siginfo_t wsiginfo;
	char *str_status;

	pr_out("[SIGNAL] Receive SIGCHLD signal");

	optflags = WNOHANG | WEXITED | WSTOPPED | WCONTINUED;
	wsiginfo = (siginfo_t) { .si_pid = 0 };

	while (true)
	{
		if (waitid(P_ALL, 0x00, &wsiginfo, optflags) == 0
				&& wsiginfo.si_pid != 0)
		{
			switch (wsiginfo.si_code)
			{
			case CLD_EXITED:
				str_status = "Exited";
				break;

			case CLD_KILLED:
				str_status = "Killed";
				break;

			case CLD_DUMPED:
				str_status = "Dumped";
				break;

			case CLD_STOPPED:
				str_status = "Stopped";
				break;

			case CLD_CONTINUED:
				str_status = "Continue";
				break;

			default:
				str_status = "si_code";
				break;
			}

			pr_out("child pid (%d) %s(%d)",
					wsiginfo.si_pid, str_status, wsiginfo.si_status);
		}
	}
}
