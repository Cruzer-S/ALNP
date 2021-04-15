#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/shm.h>

#define SZ_SHM_SEGMENT 4096

int sysv_shmget(void **ret, char *tok, key_t shm_fixkey, int size, int user_mode);
int sysv_shmrm(int shm_id);

int main(void)
{
	int shm_id;
	char *shm_ptr;
	int n_read = 0;
	size_t n_input = 128;
	char *p_input = (char *) malloc(n_input);

	printf("c: Create shared memory without key.\n");
	printf("number: attach shared memory with IPC id number.\n>>");
	if ( (n_read = (int) getline(&p_input, &n_input, stdin)) == -1)
		exit(EXIT_FAILURE);

	if (p_input[0] == 'c') {
		shm_id = sysv_shmget((void **) &shm_ptr, NULL, IPC_PRIVATE, SZ_SHM_SEGMENT, 0664);
		if (shm_id == -1)
			exit(EXIT_FAILURE);
	} else {
		shm_id = atoi(p_input);
		if ( (shm_ptr = (char *) shmat(shm_id, 0, 0)) == (char *) -1)
			exit(EXIT_FAILURE);
	}

	printf("* SHM IPC id(%d) PID(%d)\n", shm_id, getpid());
	printf("\'*\' Print current shm.\n\'.\' Exit. otherwise input text to shm.\n");
	printf("\'?\' print shm info\n");

	while (true)
	{
		printf("\n>>");
		if ( (n_read = (int) getline(&p_input, &n_input, stdin)) == -1)
			return -1;

		if (p_input[0] == '.') {
			break;
		} else if (p_input[0] == '?') {
			struct shmid_ds shm_ds;
			if (shmctl(shm_id, IPC_STAT, &shm_ds) == -1)
				return -1;

			printf("size(%zu) # of attach(%ld)\n",
					shm_ds.shm_segsz, shm_ds.shm_nattch);
			printf("shm_cpid(%d) shm_lpid(%d) \n",
					shm_ds.shm_cpid, shm_ds.shm_lpid);
		} else if (p_input[0] == '*') {
			printf("shm -> \'%.*s\'\n", SZ_SHM_SEGMENT, shm_ptr);
		} else {
			memcpy(shm_ptr, p_input, n_read);
		}
	}

	printf("* Would you remove shm (IPC id: %d) (y/n)", shm_id);
	if ( (n_read = (int)getline(&p_input, &n_input, stdin)) == -1)
		return -1;

	if (p_input[0] == 'y')
		sysv_shmrm(shm_id);

	return 0;
}

int sysv_shmget(void **ret, char *tok, key_t shm_fixkey, int size, int user_mode)
{
	key_t shm_key;
	int shm_id;
	char buf_err[128];

	if (ret == NULL || size < 0)
		return -1;

	if (tok != NULL) {
		if ((shm_key = ftok(tok, 1234)) == -1)
			return -1;
	} else shm_key = shm_fixkey;

	if ( (shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | user_mode)) == -1) {
		if (errno != EEXIST)
			return -1;

		shm_id = shmget(shm_key, 0, 0);
	}

	if (shm_id == -1) {
		strerror_r(errno, buf_err, sizeof(buf_err));
		fprintf(stderr, "FAIL: shmget(): %s [%d]\n", buf_err, __LINE__);
		return -1;
	}

	if ((*ret = shmat(shm_id, 0, 0)) == NULL) {
		strerror_r(errno, buf_err, sizeof(buf_err));
		fprintf(stderr, "FAIL: shmat(): %s [%d]\n", buf_err, __LINE__);
		return -1;
	}

	return shm_id;
}

int sysv_shmrm(int shm_id)
{
	int ret;

	if ( (ret = shmctl(shm_id, IPC_RMID, NULL)) == -1) {
		fprintf(stderr, "FAIL: shmctl(): %s [%s:%d]\n", strerror(errno), __FUNCTION__, __LINE__);
		return -1;
	}

	return ret;
}
