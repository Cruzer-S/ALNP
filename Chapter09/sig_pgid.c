#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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
	int i;
	struct sigaction sa_usr1, sa_usr2;

	memset(&sa_usr1, 0x00, sizeof(struct sigaction));
	sa_usr1.sa_handler = sa_handler_usr;
	sigfillset(&sa_usr1.sa_mask);

	memset(&sa_usr2, 0x00, sizeof(struct sigaction));
	sa_usr2.sa_handler = sa_handler_usr;
	sigfillset(&sa_usr2.sa_mask);

	sigaction(SIGUSR1, &sa_usr1, NULL);
	sigaction(SIGUSR2, &sa_usr2, NULL);

	pr_out("Parent (PID %d) (PGID %d) (SID %d)", getpid(), getpgid(0), getsid(0));

	for (i = 0; i < 3; i++) 
	{
		if (fork() == 0) {
			pr_out("\tChild (PID %d) (PGID %d) (SID %d)", getpid(), getpgid(0), getsid(0));
			break;
		}
	}


	while (true)
	{
		pause();
		pr_out("(PID %d) Receive SIGNAL...", getpid());
	}

	return 0;
}

void sa_handler_usr(int signum)
{
	switch (signum)
	{
	case SIGUSR1:
		pr_out("-- (PID %d) (PGID %d) (SID %d)", getpid(), getpgid(0), getsid(0));
		break;

	case SIGUSR2:
		if (getpid() != getpgid(0))
			setpgid(0, 0);
		else
			setpgid(0, getppid());

		pr_out("-- (PID %d) (PGID %d) (SID %d)", getpid(), getpgid(0), getsid(0));
		break;
	}
}
