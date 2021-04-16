#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define PATH_FIFO "/tmp/my_fifo"

int main(void)
{
	int fd, rc_write, rc_getline;
	char *p_buf = NULL;
	size_t len_buf = 0;

	if ((fd = open(PATH_FIFO, O_WRONLY, 0644)) == -1)
		exit(EXIT_FAILURE);

	while (true)
	{
		printf("To FIFO >> ");
		fflush(stdout);
		if ( (rc_getline = getline(&p_buf, &len_buf, stdin)) == -1)
			exit(EXIT_FAILURE);

		if (p_buf[rc_getline - 1] == '\n') rc_getline--;

		if ( (rc_write = write(fd, p_buf, strlen(p_buf))) == -1)
			exit(EXIT_FAILURE);

		printf("* Writing %d bytes...\n", rc_write);
		free(p_buf);
		p_buf = NULL;
	}

	return 0;
}
