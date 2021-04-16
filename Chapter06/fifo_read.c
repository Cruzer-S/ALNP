#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define PATH_FIFO "/tmp/my_fifo"

int main(void)
{
	int fd, n_read = 0;
	char a_buf[0xFF];

	if (mkfifo(PATH_FIFO, 0644) == -1)
		if (errno != EEXIST)
			exit(EXIT_FAILURE);

	if ( (fd = open(PATH_FIFO, O_RDONLY, 0644)) == -1)
		exit(EXIT_FAILURE);

	while (true)
	{
		if ((n_read = read(fd, a_buf, sizeof(a_buf))) == -1)
			exit(EXIT_FAILURE);

		if (n_read == 0)
			exit(EXIT_FAILURE);

		printf("[%1$d byte] %2$.*1$s\n", n_read, a_buf);
	}

	return 0;
}
