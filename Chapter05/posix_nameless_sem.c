#include <stdio.h>
#include <stdlib.h>


#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_CHILD_THREAD 30

typedef struct thread_arg {
	pthread_t tid;
	int idx;
} thread_arg;

void *start_child(void *);

thread_arg thd_arg[MAX_CHILD_THREAD];
sem_t psem;

int main(int argc, char *argv[])
{
	int i, n_count, sem_value;

	if (argc != 2) {
		printf("%s [counter]\n", argv[0]);
		exit(EXIT_FAILURE);
	} else n_count = atoi(argv[1]);

	if (sem_init(&psem, 0, n_count) == -1) {
		perror("sem_init() error");
		exit(EXIT_FAILURE);
	}

	sem_getvalue(&psem, &sem_value);
	printf("[%d] sem_getvalue = %d\n", getpid(), sem_value);
	for (i = 0; i < MAX_CHILD_THREAD; i++)
	{
		printf("[%d] iteration(%d): Atomically decrease\n", getpid(), i);
		sem_wait(&psem);
		thd_arg[i].idx = i;
		if (pthread_create(&thd_arg[i].tid, NULL, start_child, &thd_arg[i]) != 0)
			exit(EXIT_FAILURE);

		usleep(10000);
	}

	for (i = 0; i < MAX_CHILD_THREAD; i++)
		pthread_join(thd_arg[i].tid, NULL);

	sem_getvalue(&psem, &sem_value);
	printf("[%d] sem_getvalue: %d\n", getpid(), sem_value);
	if (sem_destroy(&psem) == -1)
		exit(EXIT_FAILURE);

	return 0;
}

void *start_child(void *arg)
{
	thread_arg *t_arg = (thread_arg *) arg;
	if (t_arg->idx == 11)
		return NULL;

	printf("\t[Child thread:%d] sleep(1)\n", t_arg->idx);
	sleep(1);

	sem_post(&psem);

	return 0;
}
