#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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
	struct sigaction sa_usr1;
	struct sigaction sa_usr2;

	memset(&sa_usr1, 0x00, sizeof(struct sigaction));
	sa_usr1.sa_handler = sa_handler_usr;
	sigfillset(&sa_usr1.sa_mask);

	memset(&sa_usr2, 0x00, sizeof(struct sigaction));
	sa_usr2.sa_handler = sa_handler_usr;
	sigemptyset(&sa_usr2.sa_mask);

	sigaction(SIGUSR1, &sa_usr1, NULL);
	sigaction(SIGUSR2, &sa_usr2, NULL);

	pr_out("[MAIN] signal-handler installed [pid %d]", getpid());

	while (true)
	{
		pause();
		pr_out("[MAIN] receive signal...\n");
	}

	return 0;
}

void sa_handler_usr(int signum)
{
	int i;

	for (i = 0; i < 10; i++)
	{
		pr_out("[signal %s] %d sec.", signum == SIGUSR1 ? "USR1" : "USR2", i);
		sleep(1);
	}
}
