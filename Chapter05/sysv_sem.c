#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>
#include <sys/sem.h>

#if defined(__GNU_LIBRARY__) && !(_SEM_SEMUN_UNDEFINED)
	/* union semun is defined by including <sys/sem.h> */
#else
	/* according to X/OPEN we have to define it ourselves */
	union semun {
		int val;
		struct semid_ds *buf;
		unsigned short int *array;
		struct seminfo *__buf;
	};
#endif

int sysv_semget(char *tok, key_t sem_fixkey, int n_sem, int sem_value, int user_mode);
int sysv_semwait(int sem_id, int sem_idx);
int sysv_sempost(int sem_id, int sem_idx);
int sysv_semzwait(int sem_id, int sem_idx);
int sysv_semrm(int sem_id);
int sysv_semval(int sem_id, int sem_idx);

int handle_criticalsect(int sem_id, int idx_child);

int main(int argc, char *argv[])
{
	int i, n_child, status, sem_id;

	if (argc != 2)
		exit(EXIT_FAILURE);

	n_child = atoi(argv[1]);
	sem_id = sysv_semget(NULL, 0x12340001, 1, 1, 0660);
	for (i = 0; i < n_child; i++) {
		switch (fork()) {
		case 0:
			handle_criticalsect(sem_id, i);
			exit(EXIT_SUCCESS);
			break;

		case -1:
			exit(EXIT_FAILURE);
			break;

		default:
			break;
		}

		usleep(100000);
	}

	for (i = 0; i < n_child; i++)
		waitpid(-1, &status, 0);

	if (sysv_semrm(sem_id) == -1)
		exit(EXIT_FAILURE);

	return 0;
}

int handle_criticalsect(int sem_id, int idx_child)
{
	printf("[Child:%d] #1: semval(%d) semncnt(%d)\n", idx_child,
			semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));

	if (sysv_semwait(sem_id, 0) < 0) {
		perror("sysv_semwait() error");
		return -1;
	}

	if (idx_child == 2)
		abort();

	sleep(2);

	printf("[Child:%d] #2 semval(%d) semncnt(%d)\n", idx_child,
			semctl(sem_id, 0, GETVAL), semctl(sem_id, 0, GETNCNT));

	if (sysv_sempost(sem_id, 0) < 0) {
		perror("sysv_sempost() error");
		return -1;
	}

	printf("\t[Child:%d] Exiting\n", idx_child);

	return 0;
}

int sysv_semwait(int sem_id, int sem_idx)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = sem_idx;
	sem_buf.sem_flg = SEM_UNDO;
	sem_buf.sem_op = -1;

	if (semop(sem_id, &sem_buf, 1) == -1)
		return -1;

	return 0;
}

int sysv_sempost(int sem_id, int sem_idx)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = sem_idx;
	sem_buf.sem_flg = SEM_UNDO;
	sem_buf.sem_op = 1;

	if (semop(sem_id, &sem_buf, 1) == -1)
		return -1;

	return 0;
}

int sysv_semzwait(int sem_id, int sem_idx)
{
	struct sembuf sem_buf;

	sem_buf.sem_num = sem_idx;
	sem_buf.sem_flg = 0;
	sem_buf.sem_op = 0;

	if (semop(sem_id, &sem_buf, 1) == -1)
		return -1;

	return 0;
}

int sysv_semget(char *tok, key_t sem_fixkey, int n_sem, int sem_value, int user_mode)
{
	int sem_id, i;
	union semun semun;
	unsigned short int *arr_semval = NULL;
	key_t sem_key;

	if (tok != NULL) {
		if ( (sem_key = ftok(tok, 1234)) == -1)
			return -1;

	} else sem_key = sem_fixkey;

	if ( (sem_id = semget(sem_key, n_sem, IPC_CREAT | IPC_EXCL | user_mode)) == -1) {
		if (errno == EEXIST) {
			sem_id = semget(sem_key, n_sem, 0);
			return sem_id;
		}
	}

	if (sem_id == -1)
		return -1;

	semun.val = sem_value;
	for (i = 0; i < n_sem; i++)
		if (semctl(sem_id, i, SETVAL, semun) == -1)
			return -1;

	return sem_id;
}

int sysv_semrm(int sem_id)
{
	if (semctl(sem_id, 0, IPC_RMID) == -1)
		return -1;

	return 0;
}

int sysv_semval(int sem_id, int sem_idx)
{
	int semval;

	if ( (semval = semctl(sem_id, sem_idx, GETVAL)) == -1)
		return -1;

	return semval;
}
