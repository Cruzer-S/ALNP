#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/signal.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

void *start_sigthread(void *);
void *start_thread(void *);

struct thread_arg {
	pthread_t pt_id;
	void *(*func)(void *);
} thd_arg[NUM_THREADS] = {
	{ 0, start_sigthread },
	{ 0, start_thread },
	{ 0, start_thread },
	{ 0, NULL }
};

void clean_thread(struct thread_arg *t_arg);
void sa_handler_usr(int signum);

int main(void)
{
	int i;
	sigset_t sigset_mask;

	sigfillset(&sigset_mask);
	sigdelset(&sigset_mask, SIGINT);

	pthread_sigmask(SIG_SETMASK, &sigset_mask, NULL);
	pr_out("* Process PID %d", getpid());

	for (i = 0; i < NUM_THREADS && thd_arg[i].func; i++) {
		if (pthread_create(&thd_arg[i].pt_id, NULL, thd_arg[i].func, (void *) &thd_arg[i]))
			pr_crt("[MAIN] FAIL: pthread_create()");

		pr_out("Create thread: tid %lu", thd_arg[i].pt_id);
	}

	clean_thread(thd_arg);

	return 0;
}

void *start_sigthread(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;
	sigset_t sigset_mask;
	int signum, ret_errno;

	pr_out("* Start signal thread: tid %lu", (long) pthread_self());
	sigemptyset(&sigset_mask);

	sigaddset(&sigset_mask, SIGUSR1);
	sigaddset(&sigset_mask, SIGUSR2);
	sigaddset(&sigset_mask, SIGTERM);

	while (true)
	{
		if ((ret_errno = sigwait(&sigset_mask, &signum))) {
			pr_out("FAIL: sigwait %s", strerror(ret_errno));
			continue;
		}

		switch (signum) {
		case SIGUSR1: case SIGUSR2:
			sa_handler_usr(signum);
			break;

		case SIGTERM:
			pr_out("[SIGNAL] SIGTERM");
			exit(EXIT_SUCCESS);
			break;

		default:
			break;
		}
	}

	return t_arg;
}

void sa_handler_usr(int signum)
{
	for (int i = 0; i < 5; i++)
	{
		pr_out("\t[%ld] signal(%s) %d sec.", pthread_self(),
				signum == SIGUSR1 ? "USR1" : "USR2", i);

		sleep(1);
	}
}

void *start_thread(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;

	for (int i = 0; true; i++)
		sleep(1);

	return t_arg;
}

void clean_thread(struct thread_arg *t_arg)
{
	struct thread_arg *t_arg_ret;

	for (int i = 0; i < NUM_THREADS && t_arg->func; i++, t_arg++)
	{
		pthread_join(t_arg->pt_id, (void **) &t_arg_ret);
		pr_out("+ Thread id (%lu)", t_arg->pt_id);
	}
}
