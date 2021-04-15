#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <mqueue.h>

#define NAME_POSIX_MQ "/my_mq"
#define LEN_RBUF 512

mqd_t mq_fd;
int start_msq_sender(char *srcfile);
int start_msq_receiver(void);

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
	int n_recv;
	struct mq_attr mq_attrib;
	char *p_buf;

	mq_getattr(mq_fd, &mq_attrib);

	if ( (p_buf = malloc(mq_attrib.mq_msgsize)) == NULL)
		return -1;

	while (true) {
		if ( (n_recv = mq_receive(mq_fd, p_buf, mq_attrib.mq_msgsize, NULL)) == -1) {
			perror("FAIL: mq_receive()");
			return -1;
		}

		printf("+ Recv(%.*s)\n", n_recv, p_buf);
	}

	return 0;
}
