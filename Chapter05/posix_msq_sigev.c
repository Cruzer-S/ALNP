#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/signal.h>

#include <mqueue.h>

#define NAME_POSIX_MQ "/my_mq"
#define LEN_RBUF 512

mqd_t mq_fd;
int start_msq_sender(char *srcfile);
int start_msq_receiver(void);
void chk_rt(int sig, siginfo_t *si, void *data);

int main(int argc, char *argv[])
{
	char buf_err[128];
	struct mq_attr mq_attrib = { .mq_maxmsg = 10, .mq_msgsize = 1024 };

	if (argc < 2) {
		printf("Usage: %s <sender filename | receiver | unlink>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("* POSIX Message queue test program\n");
	if (argv[1][0] == 'u') {
		printf("\tRemove MQ: %s\n", NAME_POSIX_MQ);
		mq_unlink(NAME_POSIX_MQ);
		exit(EXIT_SUCCESS);
	}


	if ((mq_fd = mq_open(NAME_POSIX_MQ, O_RDWR | O_CREAT | O_EXCL, 0660, &mq_attrib)) > 0) {
		printf("* Create MQ\n");
	} else {
		if (errno != EEXIST) {
			strerror_r(errno, buf_err, sizeof(buf_err));
			printf("FAIL: mq_open(): %s\n", buf_err);
			exit(EXIT_FAILURE);
		}

		if ((mq_fd = mq_open(NAME_POSIX_MQ, O_RDWR)) == (mqd_t) -1) {
			strerror_r(errno, buf_err, sizeof(buf_err));
			printf("FAIL: mq_open(): %s\n", buf_err);
			exit(EXIT_FAILURE);
		}
	}

	switch (argv[1][0]) {
	case 's':
		printf("+ Sender start transaction with the file (%s).\n", argv[2]);
		(void) start_msq_sender(argv[2]);
		printf("+ Finished. Press any key will exit.\n");
		break;

	case 'r':
		printf("+ Receiver waiting for message.\n");
		(void) start_msq_receiver();
		break;

	default:
		fprintf(stderr, "* Unkown option, use sender or receiver\n");
		break;
	}

	mq_close(mq_fd);

	return 0;
}

int start_msq_sender(char *srcfile)
{
	FILE *fp_srcfile;
	char rbuf[LEN_RBUF];
	int len_rbuf;

	if ((fp_srcfile = fopen(srcfile, "r")) == NULL) {
		perror("FAIL: fopen()");
		return -1;
	}

	while (!feof(fp_srcfile)) {
		if (fgets(rbuf, sizeof(rbuf), fp_srcfile) == NULL)
			break;

		len_rbuf = strnlen(rbuf, sizeof(rbuf)) - 1;
		printf("\t- Send (text:%.*s)\n", len_rbuf, rbuf);
		if (mq_send(mq_fd, rbuf, len_rbuf, 0) == -1) {
			perror("FAIL: mq_send()");
		}
	}

	fclose(fp_srcfile);

	return 0;
}

int start_msq_receiver(void)
{
	int n_recv, i = 0;
	char *p_buf;
	struct sigevent sigev_noti;
	struct sigaction sa_rt;
	struct mq_attr mq_at;

	sa_rt.sa_sigaction = chk_rt;
	sigemptyset(&sa_rt.sa_mask);
	sa_rt.sa_flags = SA_SIGINFO | SA_RESTART;

	sigaction(SIGRTMIN + 2, &sa_rt, NULL);

	memset(&sigev_noti, 0x00, sizeof(struct sigevent));
	sigev_noti.sigev_notify = SIGEV_SIGNAL;
	sigev_noti.sigev_signo = SIGRTMIN + 2;

	mq_getattr(mq_fd, &mq_at);

	if ( (p_buf = malloc(mq_at.mq_msgsize)) == NULL)
		return -1;

	while (true) {
		i++;

		mq_getattr(mq_fd, &mq_at);
		printf("+---- MQ Status size/cur#/max# = (%ld/%ld/%ld)\n",
				mq_at.mq_msgsize, mq_at.mq_curmsgs, mq_at.mq_maxmsg);

		sigev_noti.sigev_value.sival_int = i;
		if (mq_notify(mq_fd, &sigev_noti) == -1) {
			if (errno == EBUSY) {
				perror("FAIL: mq_notify(): EBUSY");
			} else {
				perror("FAIL: mq_notify()");
				return -1;
			}
		}

		if ( (n_recv = mq_receive(mq_fd, p_buf, mq_at.mq_msgsize, 0)) == -1) {
			perror("FAIL: mq_receive()");
			return -1;
		}

		printf("+ [%02d] Recv(%.*s)\n", i, n_recv, p_buf);
	}

	return 0;
}

void chk_rt(int sig, siginfo_t *si, void *data)
{
	printf("[SIGRT] si_code(%d) si_band(%lx) si_value(%d)\n",
			si->si_code, si->si_band, si->si_value.sival_int);
}
