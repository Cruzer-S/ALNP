#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
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
#define MAX_ITEMS 64
#define PATH_FIFO "/tmp/my_fifo"

struct workqueue {
	int item[MAX_ITEMS];
	int idx;
	int cnt;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
} *wq;

void *tfunc_a(void *);
void *tfunc_b(void *);

struct thread_arg {
	pthread_t tid;
	int idx;
	void *(*func)(void *);
} t_arg[] = {
	{0, 0, tfunc_a},
	{0, 0, tfunc_b},
	{0, 0, tfunc_b},
	{0, 0, tfunc_b},
	{0, 0, NULL}
};

int push_item(struct workqueue *wq, const char *item, int cnt);
int pop_item(struct workqueue *wq, int *item);
int process_job(int *);

void clean_thread(struct thread_arg *);

int main(int argc, char *argv[])
{
	int i;

	if ((wq = calloc(1, sizeof(struct workqueue))) == NULL)
		pr_crt("calloc() error");

	pthread_mutex_init(&wq->mutex, NULL);
	pthread_cond_init(&wq->cv, NULL);

	for (i = 0; i < NUM_THREADS && t_arg[i].func != NULL; i++)
	{
		t_arg[i].idx = i;

		if (pthread_create(&t_arg[i].tid, NULL, t_arg[i].func, (void *) &t_arg[i]))
			exit(EXIT_FAILURE);

		pr_out("pthread_create: tid = %lu", t_arg[i].tid);
	}

	clean_thread(t_arg);

	return 0;
}

void *tfunc_a(void *arg)
{
	int fd, ret_read = 0;
	char buf[MAX_ITEMS / 2];

	pr_out(">> Thread (A) started!");

	if (mkfifo(PATH_FIFO, 0644) == -1)
		if (errno != EEXIST)
			pr_crt("[A] FAIL: mkfifo()");

	if ((fd = open(PATH_FIFO, O_RDONLY, 0644)) == -1)
		pr_crt("FAIL: open()");

	while (true)
	{
		if ((ret_read = read(fd, buf, sizeof(buf))) == -1)
			pr_crt("[A] FAIL: read()");

		if (ret_read == 0)
			pr_crt("[A] Broken pipe");

		push_item(wq, buf, ret_read);
		pr_out("[A] cond_signal");

		pthread_cond_broadcast(&wq->cv);
	}

	return NULL;
}

void *tfunc_b(void *arg)
{
	int item;

	pr_out(">> Thread (B) started!");

	while (true)
	{
		pop_item(wq, &item);
		process_job(&item);
	}

	return NULL;
}

int push_item(struct workqueue *wq, const char *item, int cnt)
{
	int i, j;

	pthread_mutex_lock(&wq->mutex);

	for (i = 0, j = (wq->idx + wq->cnt) % MAX_ITEMS;
		 i < cnt;
		 i++, j++, wq->cnt++)
	{
		if (wq->cnt == MAX_ITEMS) {
			pr_err("[Q:%d,%d] queue full: wq(idx,cnt=%d,%d)",
					i, j, wq->idx, wq->cnt);
			break;
		}

		if (j == MAX_ITEMS)
			j = 0;

		wq->item[j] = (int) item[i];
		pr_out("[Q:%d,%d] push (idx,cnt=%d,%d): item=(%c)", i, j, wq->idx, wq->cnt, item[i]);
	}

	pthread_mutex_unlock(&wq->mutex);

	return i;
}

int pop_item(struct workqueue *wq, int *item)
{
	pthread_mutex_lock(&wq->mutex);

	while (true)
	{
		if (wq->cnt > 0) {
			if (wq->idx == MAX_ITEMS)
				wq->idx = 0;

			*item = wq->item[wq->idx];
			wq->idx++;
			wq->cnt--;

			pr_out("[B] pop(%d,%d) item(%c) (tid=%ld)",
					wq->idx, wq->cnt, (char) *item, pthread_self() % 10);

			break;
		} else {
			pr_out("[B] cond_wait (tid=%ld)", pthread_self() % 10);
			pthread_cond_wait(&wq->cv, &wq->mutex);
			pr_out("[B] Wake up (tid=%ld)", pthread_self() % 10);
		}
	}

	pr_out("[B] Unlock mutex (tid=%ld)", pthread_self());

	pthread_mutex_unlock(&wq->mutex);

	return 0;
}

int process_job(int *item)
{
	pr_out("[A] item=%d", *item);

	return 0;
}

void clean_thread(struct thread_arg *t_arg)
{
	int i;
	struct thread_arg *t_arg_ret;

	for (i = 0; i < NUM_THREADS && t_arg[i].func != NULL; i++)
	{
		pthread_join(t_arg->tid, (void **) &t_arg_ret);
		pr_out("pthread_join: %d - %lu", t_arg->idx, t_arg->tid);
	}
}
