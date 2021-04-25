#include <stdio.h>
#include <string.h>
#include <errno.h>
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

#define NUM_THREADS 4

struct thread_arg {
	pthread_t tid;
	int idx;
};

void *start_thread(void *);
void clean_thread(struct thread_arg *);

struct thread_arg *t_arg;

pthread_mutex_t mutex;
pthread_mutexattr_t mutexattr;

int main(void)
{
	int i, ret;

	t_arg = (struct thread_arg *) calloc(NUM_THREADS, sizeof(struct thread_arg));
	pthread_mutexattr_init(&mutexattr);

#if defined(NORMAL_MUTEX)
	pr_out("NORMAL_MUTEX");
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_NORMAL);
#elif defined(RECURSIVE_MUTEX)
	pr_out("RECURSIVE MUTEX");
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
#elif defined(ERRORCHECK_MUTEX)
	pr_out("ERRORCHECK MUTEX");
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
#endif

	pthread_mutex_init(&mutex, &mutexattr);

	for (i = 0; i < NUM_THREADS; i++)
	{
		t_arg[i].idx = i;

		if ((ret = pthread_create(&t_arg[i].tid, NULL, start_thread, (void *) &t_arg[i])))
			pr_crt("pthread_create: %s ", strerror(errno));

		pr_out("pthread_create: idx = %d, tid = %lu", t_arg[i].idx, t_arg[i].tid);
	}

	clean_thread(t_arg);

	return 0;
}

void *start_thread(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;
	int ret;

	if ((ret = pthread_mutex_lock(&mutex))) {
		if (ret == EDEADLK) {
			pr_err("\tlock: EDEADLK detected");
		} else {
			pr_err("\tFail: thread_mutex_lock()");
		}
	}

	pr_out("[thread] idx(%d) tid(%ld)", t_arg->idx, pthread_self());

	if (t_arg->idx > 1) {
		pr_out("Try to lock Mutex already locked");
		if ((ret = pthread_mutex_lock(&mutex))) {
			if (ret == EDEADLK) {
				pr_err("\tlock: EDEADLK detected");
			} else {
				pr_err("\tFail: thread_mutex_lock()");
			}
		}
	}

	if ((ret = pthread_mutex_unlock(&mutex))) {
		pr_err("\t unlock: (errno = %s)", strerror(ret));
	}

	return t_arg;
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
