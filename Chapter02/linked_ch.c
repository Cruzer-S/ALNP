#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>

#include <sys/wait.h>

char file_log[64];
void cat_logfile(void);

int main(int argc, char *argv[])
{
	int fd0, fd1;
	FILE *fp0;
	char buf[64];

	snprintf(file_log, sizeof(file_log), "%s.log", argv[0]);
	if ( (fd0 = open(file_log, O_CREAT | O_TRUNC | O_RDWR, 0644)) == -1)
		return -1;

	write(fd0, "1234567890abcdefghij", 20);
	cat_logfile();

	if ( (fd1 = dup(fd0)) == -1) {
		close(fd0);
		return -1;
	}

	write(fd1, "OPQRSTU", 7);
	cat_logfile();

	if ( (fp0 = fdopen(fd1, "r+")) == NULL) {
		close(fd0);
		return -1;
	}

	printf("\tfd0(%d) fd1(%d)\n", fd0, fd1);

	lseek(fd1, 2, SEEK_SET);
	write(fd1, ",fd1", 5);
	cat_logfile();

	write(fd0, ",fd0", 5);
	cat_logfile();

	fread(buf, 5, 1, fp0);
	printf("read buf=\"%.5s\"\n", buf);
	fwrite("(^o^)", 5, 1, fp0);
	fflush(fp0);
	cat_logfile();

	fclose(fp0);
	close(fd1);
	close(fd0);

	return 0;
}

void cat_logfile(void)
{
	static int cnt;

	char *argv_child[] = { "cat", file_log, NULL };
	printf("%d={", cnt++);
	fflush(stdout);
	posix_spawnp(NULL, argv_child[0], NULL, NULL, argv_child, NULL);
	wait(NULL);

	printf("}\n");
}
