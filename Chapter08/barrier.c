#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

#define NUM_THREADS 5

pthread_barrier_t pt_barrier;

struct thread_arg {
	pthread_t tid;
	int idx;
} t_arg[NUM_THREADS];

void *start_thread(void *);
void clean_thread(struct thread_arg *);

#define GET_TIME0(a) get_time0(a, sizeof(a))

char *get_time0(char *buf, size_t sz_buf);

int main(int argc, char *argv[])
{
	int i;

	pthread_barrier_init(&pt_barrier, NULL, NUM_THREADS);
	for (i = 0; i < NUM_THREADS; i++) {
		t_arg[i].idx = i;

		if (pthread_create(&t_arg[i].tid, NULL, start_thread, (void *) &t_arg[i]))
			pr_crt("Error: pthread_create()");
	}

	clean_thread(t_arg);

	pthread_barrier_destroy(&pt_barrier);

	return 0;
}

void *start_thread(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;

	char ts_now[20];
	int ret;

	pr_out("[Thread:%d] [%s] sleep(%d)",
			t_arg->idx, GET_TIME0(ts_now), t_arg->idx + 2);

	sleep(t_arg->idx + 2);

	ret = pthread_barrier_wait(&pt_barrier);

	if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
		pr_out("[Thread:%d] PTHREAD_BARRIER_SERIAL_THREAD", t_arg->idx);

	pr_out("\t[Thread:%d] [%s] wake up", t_arg->idx, GET_TIME0(ts_now));

	pthread_exit(t_arg);
}

char *get_time0(char *buf, size_t sz_buf)
{
#define STR_TIME_FORMAT "%H:%M:%S"
	time_t t0;
	struct tm tm_now;

	if (buf == NULL)
		return NULL;

	if (time(&t0) == ((time_t) -1))
		return NULL;

	localtime_r(&t0, &tm_now);

	if (strftime(buf, sz_buf, STR_TIME_FORMAT, &tm_now) == 0)
		return NULL;

	return buf;
}

void clean_thread(struct thread_arg *t_arg)
{
	struct thread_arg *t_arg_ret;
	int i;
	
	for (i = 0; i < NUM_THREADS; i++, t_arg++) {
		pthread_join(t_arg->tid, (void **) &t_arg_ret);
		pr_out("pthread_join: %d - %lu", t_arg->idx, t_arg->tid);
	}
}
