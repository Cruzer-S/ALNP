#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/msg.h>

#define LEN_MQ_MTEXT 512

int msg_id;

struct mq_buf {
	long mtype;
	char mtext[LEN_MQ_MTEXT];
};

int sysv_msgget(char *tok, key_t msg_fixkey, int user_mode);
int sysv_msgrm(int msg_id);

int start_msq_sender(char *srcfile);
int start_msq_receiver(long mtype);

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: %s <sender | receiver> <filename or mtype>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((msg_id = sysv_msgget(argv[0], 0, 0664)) == -1)
		exit(EXIT_FAILURE);

	printf("* Message queue test program (ID:%d)\n", msg_id);
	switch (argv[1][0]) {
	case 's':
		printf("+ Sender start with file(%s)\n", argv[2]);
		(void) start_msq_sender(argv[2]);
		printf("+ Finishied. (Ctrl-C:Exit) (Press any key: Remove MQ)\n");
		getchar();
		break;

	case 'r':
		printf("+ Receiver start with type(%s)\n", argv[2]);
		(void) start_msq_receiver(atol(argv[2]));
		break;

	default:
		fprintf(stderr, "* Unkown option, use sender or receiver\n");
		break;
	}
	return 0;
}

int sysv_msgget(char *tok, key_t msg_fixkey, int user_mode)
{
	key_t msg_key;
	int msg_id;
	char buf_err[128];

	if (tok != NULL) {
		if ((msg_key = ftok(tok, 1234)) == -1)
			return -1;
	} else msg_key = msg_fixkey;

	if ( (msg_id = msgget(msg_key, IPC_CREAT | IPC_EXCL | user_mode)) == -1) {
		if (errno == EEXIST)
			msg_id = msgget(msg_key, 0);
	}

	if (msg_id == -1) {
		strerror_r(errno, buf_err, sizeof(buf_err));
		fprintf(stderr, "FAIL: msgctl [%s:%d]\n", __FUNCTION__, __LINE__);
	}

	return 0;
}

int sysv_msgrm(int msg_id)
{
	if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "FAIL: msgctl[%s:%d]\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

int start_msq_sender(char *srcfile)
{
	FILE *fp_srcfile;
	char rbuf[LEN_MQ_MTEXT];
	struct mq_buf mq_buf;
	int len_mtext;
	int number = 1;

	if ((fp_srcfile = fopen(srcfile, "r")) == NULL) {
		perror("FAIL: fopen()");
		return -1;
	}

	while (!feof(fp_srcfile))
	{
		if (fgets(rbuf, sizeof(rbuf), fp_srcfile) == NULL)
			break;

		mq_buf.mtype = number++;
		len_mtext = strnlen(rbuf, LEN_MQ_MTEXT) - 1;
		memcpy(mq_buf.mtext, rbuf, len_mtext);
		printf("\t- Send (mtype:%ld, len:%d) - (mtext:%.*s)\n",
				mq_buf.mtype, len_mtext, len_mtext, mq_buf.mtext);
		if (msgsnd(msg_id, (void *) &mq_buf, len_mtext, IPC_NOWAIT) == -1) {
			perror("FAIL: msgsnd()");
			break;
		}
	}

	fclose(fp_srcfile);

	return 0;
}

int start_msq_receiver(long mtype)
{
	int n_recv;
	struct mq_buf mq_buf;

	while (true) {
		if ( (n_recv = msgrcv(msg_id, &mq_buf, LEN_MQ_MTEXT, mtype, 0)) == -1) break;
		printf("+ Recv (mtype:%ld, len:%d) - (%.*s)\n",
				mq_buf.mtype, n_recv, n_recv, mq_buf.mtext);
	}

	return 0;
}
