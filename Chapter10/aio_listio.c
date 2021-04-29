#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <aio.h>
#include <fcntl.h>

#define NUM_AIOCB_ARRAY 5

#ifdef ASYNCHRONIZED_IO
	#include <pthread.h>
	void start_sigev_aio(sigval_t arg);
	pthread_attr_t pt_attr;
	pthread_barrier_t pt_barrier;
#endif

int main(int argc, char *argv[])
{
	int fd_rd, fd_wr;
	int i = 0, tot_len = 0, ret;
	char ofname[0xFF], rbuf[NUM_AIOCB_ARRAY][1024 * 256];

	struct aiocb aio_blk[NUM_AIOCB_ARRAY];
	struct aiocb *aiolist[] = {
		&aio_blk[0], &aio_blk[1],
		&aio_blk[2], &aio_blk[3], &aio_blk[4]
	};

#ifdef ASYNCHRONIZED_IO
	struct sigevent sigev = { .sigev_notify = SIGEV_THREAD };
#endif

	if (argc != 2) {
		printf("usage: %s <source>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sprintf(ofname, "%s.copy", argv[1]);
	printf("file copy: %s =>>> %s\n", argv[1], ofname);
	if ((fd_rd = open(argv[1], O_RDONLY, 0)) == -1)
		exit(EXIT_FAILURE);

	if ((fd_wr = open(ofname, O_CREAT | O_WRONLY, 0644)) == -1)
		exit(EXIT_FAILURE);

	memset(aio_blk, 0x00, sizeof(struct aiocb) * NUM_AIOCB_ARRAY);
	while (true)
	{
		ret = read(fd_rd, rbuf[i], sizeof(rbuf[i]));
		if (ret == -1)
			exit(EXIT_FAILURE);

		aio_blk[i].aio_fildes = fd_wr;
		aio_blk[i].aio_buf = rbuf[i];
		aio_blk[i].aio_nbytes = ret;
		aio_blk[i].aio_offset = tot_len;
		aio_blk[i].aio_lio_opcode = LIO_WRITE;

		tot_len += ret;

		i++;

		if (i == NUM_AIOCB_ARRAY || ret == 0) {
#ifdef ASYNCHRONIZED_IO
			pthread_barrier_init(&pt_barrier, NULL, 2);
			sigev.sigev_notify = SIGEV_THREAD;
			sigev.sigev_value.sival_int = tot_len;
			pthread_attr_init(&pt_attr);
			pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_DETACHED);
			sigev.sigev_notify_attributes = &pt_attr;
			sigev.sigev_notify_function = start_sigev_aio;
			lio_listio(LIO_NOWAIT, aiolist, ret == 0 ? i - 1 : i, &sigev);
			pthread_barrier_wait(&pt_barrier);
			pthread_barrier_destroy(&pt_barrier);
#else
			lio_listio(LIO_WAIT, aiolist, ret == 0 ? i - 1 : i, NULL);
#endif
			memset(aio_blk, 0x00, sizeof(struct aiocb) * NUM_AIOCB_ARRAY);
			i = 0;
		}

		if (ret == 0)
			break;
	}

	printf("> write complete\n");
	close(fd_rd);
	fsync(fd_wr);
	close(fd_wr);

	return 0;
}

#ifdef ASYNCHRONIZED_IO
void start_sigev_aio(sigval_t arg)
{
	printf("[SIGEV] thread %ld by lio_listio(len %d)\n",
			(long) pthread_self(), arg.sival_int);
	pthread_barrier_wait(&pt_barrier);
}
#endif
