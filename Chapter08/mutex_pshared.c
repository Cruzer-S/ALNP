#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/mman.h>
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

#define NUM_THREADS 6
#define SHM_SEGMENT 4096

struct thread_arg {
	pthread_t tid;
	int idx;
} *t_arg;

const char *shm_path = "/alsp_mutex";
int shm_fd;

struct shr_data {
	pid_t prev_pid;
	int prev_idx;
	time_t prev_time;
	pthread_mutex_t mutex;
} *shr_data;

void *start_thread(void *);
void clean_thread(struct thread_arg *);

int main(int argc, char *argv[])
{
	int i, ret;

	t_arg = (struct thread_arg *) calloc(NUM_THREADS, sizeof(struct thread_arg));
	if (t_arg == NULL)
		pr_crt("Error: calloc()");

	if ( (shm_fd = shm_open(shm_path, O_CREAT | O_RDWR | O_EXCL, 0660)) == -1) {
		if (errno == EEXIST)
			shm_fd = shm_open(shm_path, O_RDWR, 0660);
		else
			exit(EXIT_FAILURE);

		ftruncate(shm_fd, SHM_SEGMENT);
	}

	shr_data = (struct shr_data *) mmap((void *) 0, SHM_SEGMENT,
			PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

	if (shr_data == MAP_FAILED)
		pr_crt("Error: mmap()");

	if (argc > 1 && argv[1][0] == 'c') {
		pr_out("init mutex");

		pthread_mutexattr_t mutexattr;
		memset(shr_data, 0x00, sizeof(struct shr_data));

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&shr_data->mutex, &mutexattr);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		t_arg[i].idx = i;

		if ( (ret = pthread_create(&t_arg[i].tid, NULL, start_thread, (void *) &t_arg[i])))
			pr_err("Error: pthread_create()");
	}

	clean_thread(t_arg);

	return 0;
}

void *start_thread(void *arg)
{
	struct thread_arg *t_arg = (struct thread_arg *) arg;
	
	sleep(t_arg->idx);

	pthread_mutex_lock(&shr_data->mutex);
	pr_out("[%d, %d] => [%d, %d]",
			shr_data->prev_pid, shr_data->prev_idx,
			getpid(), t_arg->idx);

	sleep(t_arg->idx + 1);

	shr_data->prev_pid = getpid();
	shr_data->prev_idx = t_arg->idx;

	pthread_mutex_unlock(&shr_data->mutex);

	pthread_exit(t_arg);
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
