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

void sa_sigaction_usr(int signum, siginfo_t *si, void *sv);

int main(void)
{
	struct sigaction sa_usr1;
	struct sigaction sa_usr2;

	memset(&sa_usr1, 0x00, sizeof(struct sigaction));
	sa_usr1.sa_sigaction = sa_sigaction_usr;
	sigfillset(&sa_usr1.sa_mask);
	sa_usr1.sa_flags = SA_SIGINFO;

	memset(&sa_usr2, 0x00, sizeof(struct sigaction));
	sa_usr2.sa_sigaction = sa_sigaction_usr;
	sigemptyset(&sa_usr2.sa_mask);
	sa_usr2.sa_flags = SA_SIGINFO;


	sigaction(SIGUSR1, &sa_usr1, NULL);
	sigaction(SIGUSR2, &sa_usr2, NULL);

	pr_out("[MAIN] signal-handler installed [pid %d]\n", getpid());

	while (true)
	{
		pause();
		pr_out("[MAIN] receive signal... \n");
	}

	return 0;
}

void sa_sigaction_usr(int signum, siginfo_t *si, void *sv)
{
	int i;

	pr_out("\t[signo %d] [UID %d] [PID %d]", si->si_signo, si->si_uid, si->si_pid);
	for (i = 0; i < 10; i++)
	{
		pr_out("\t[signal %s] %d sec.", signum == SIGUSR1 ? "USR1" : "USR2", i);
		sleep(1);
	}
} 
