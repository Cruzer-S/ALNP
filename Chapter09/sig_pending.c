#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/signal.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)


void sa_handler_usr(int signum);

int main(void)
{
	struct sigaction sa_usr1, sa_usr2;
	sigset_t sigset_mask, sigset_oldmask, sigset_pend;
	int i;

	memset(&sa_usr1, 0x00, sizeof(struct sigaction));
	sa_usr1.sa_handler = sa_handler_usr;
	sigfillset(&sa_usr1.sa_mask);
	sigaction(SIGUSR1, &sa_usr1, NULL);

	sa_usr2.sa_handler = SIG_IGN;
	sigaction(SIGUSR2, &sa_usr2, NULL);

	sigfillset(&sigset_mask);
	sigdelset(&sigset_mask, SIGINT);

	pr_out("(PID %d)", getpid());

	while (true)
	{
		pr_out("Install signal block mask (allow only SIGINT)");

		sigprocmask(SIG_SETMASK, &sigset_mask, &sigset_oldmask);
		sleep(10);

		sigpending(&sigset_pend);
		for (i = 1; i < SIGRTMIN; i++) 
		{
			if (sigismember(&sigset_pend, i)) {
				pr_out("\tPending singal %d", i);

				switch (i) {
				case SIGUSR2:
					sa_handler_usr(SIGUSR2);
					break;

				default:
					break;
				}
			}
		}

		pr_out("Restore the previous single block mask.");
		sigprocmask(SIG_SETMASK, &sigset_oldmask, NULL);
	}

	return 0;
}

void sa_handler_usr(int signum)
{
	int i;

	for (i = 0; i < 3; i++) {
		pr_out("\tsignal (%s): %d sec.", signum == SIGUSR1 ? "USR1" : "USR2", i);
		sleep(1);
	}
}
