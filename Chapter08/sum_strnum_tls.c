#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 3
#define LEN_SUM_STR 16

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

struct thread_arg {
	pthread_t tid;
	int idx;
	char *x, *y;
} t_arg[NUM_THREADS];

void *start_func(void *);
void clean_thread(struct thread_arg *);
char *sum_strnum(const char *, const char *);

pthread_once_t once_tls_key = PTHREAD_ONCE_INIT;
pthread_key_t tls_key;

void init_tls_key(void);
void destroy_tls(void *);

int main(void)
{
	t_arg[0].idx = 0; t_arg[0].x = "1"; t_arg[0].y = "3";
	if (pthread_create(&t_arg[0].tid, NULL, start_func, &t_arg[0]) != 0)
		pr_crt("Error: pthread_create");

	t_arg[1].idx = 1; t_arg[1].x = "4"; t_arg[1].y = "4";
	if (pthread_create(&t_arg[1].tid, NULL, start_func, &t_arg[1]) != 0)
		pr_crt("Error: pthread_create");

	t_arg[2].idx = 2; t_arg[2].x = "1"; t_arg[2].y = "5";
	if (pthread_create(&t_arg[2].tid, NULL, start_func, &t_arg[2]) != 0)
		pr_crt("Error: pthread_create");

	clean_thread(t_arg);

	return 0;
}

void *start_func(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;
	char *ret_str = sum_strnum(t_arg->x, t_arg->y);

	flockfile(stdout);
	printf("%s + %s = %s (%p) \n", t_arg->x, t_arg->y, ret_str, ret_str);
	funlockfile(stdout);

	pthread_exit(t_arg);
}

char *sum_strnum(const char *s1, const char *s2)
{
	char *tls_str;

	pthread_once(&once_tls_key, init_tls_key);
	if ((tls_str = pthread_getspecific(tls_key)) == NULL) {
		tls_str = malloc(LEN_SUM_STR);
		pthread_setspecific(tls_key, tls_str);
	}

	snprintf(tls_str, LEN_SUM_STR, "%d", atoi(s1) + atoi(s2));

	return tls_str;
}

void init_tls_key(void)
{
	pthread_key_create(&tls_key, destroy_tls);
}

void destroy_tls(void *tls)
{
	flockfile(stdout);
	printf("destructor: TID(%ld) TLS(%p)\n", pthread_self(), tls);
	funlockfile(stdout);

	free(tls);
}

void clean_thread(struct thread_arg *t_arg)
{
	struct thread_arg *t_arg_ret;
	int i;

	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(t_arg->tid, (void **) &t_arg_ret);
		pr_out("pthread_join: %d - %lu", t_arg->idx, t_arg->tid);
	}
}
