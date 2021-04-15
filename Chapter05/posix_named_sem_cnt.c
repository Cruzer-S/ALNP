#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NAME_POSIX_SEM "my_psem"

const int max_child = 30;
int cur_child = max_child;
sem_t *p_psem;

int process_child(int i);
void check_abrt(int signo);

int main(int argc, char *argv[])
{
	int i, n_count, sem_value, status;
	if (argc != 2)
		exit(EXIT_FAILURE);

	n_count = atoi(argv[1]);
	p_psem = sem_open(NAME_POSIX_SEM, O_CREAT | O_EXCL, 0600, n_count);
	if (p_psem == SEM_FAILED) {
		if (errno != EEXIST)
			exit(EXIT_FAILURE);

		p_psem = sem_open(NAME_POSIX_SEM, 0);
		printf("[%d] Attach to an existed sem\n", getpid());
	} else printf("[%d] Create new sem\n", getpid());

	sem_getvalue(p_psem, &sem_value);
	printf("[%d] sem_getvalue = %d\n", getpid(), sem_value);

	signal(SIGCHLD, check_abrt);

	for (i = 0; i < max_child; i++)
	{
		printf("[%d] iteration (%d): Atomically decrease\n", getpid(), i);
		sem_wait(p_psem);

		switch ( fork() ) {
		case 0:
			process_child(i);
			sem_post(p_psem);
			exit(EXIT_SUCCESS);
			break;

		case -1:
			exit(EXIT_FAILURE);
			break;

		default:;
			break;
		}
		usleep(10000);
	}

	while (cur_child > 0);

	sem_getvalue(p_psem, &sem_value);
	printf("[%d] sem_getvalue = %d\n", getpid(), sem_value);
	if (sem_unlink(NAME_POSIX_SEM) == -1) {
		exit(EXIT_FAILURE);
	}

	return 0;
}

void check_abrt(int signo)
{
	pid_t pid_child;
	int status;

	if ( (pid_child = waitpid(-1, &status, WNOHANG)) == -1)
		exit(EXIT_FAILURE);

	if (status != EXIT_SUCCESS) {
		printf("\naborted child: increase semaphore\n");
		sem_post(p_psem);
	}

	cur_child--;
}

int process_child(int i)
{
	if (i == 11) abort();

	printf("\t[Child:%d] sleep(2)\n", i);
	sleep(1);

	return 0;
}
