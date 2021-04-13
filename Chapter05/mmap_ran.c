#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MMAP_FILENAME	"mmapfile.dat"
#define MMAP_SIZE		64

int main(void)
{
	int fd, n_write, flag_map = MAP_SHARED;
	char *p_map, a_input[100];

	if ( (fd = open(MMAP_FILENAME, O_RDWR | O_CREAT, 0664)) == -1)
		exit(EXIT_FAILURE);


	if (ftruncate(fd, MMAP_SIZE) == -1)
		exit(EXIT_FAILURE);

	if ( (p_map = mmap((void *) 0,
					   MMAP_SIZE, PROT_READ | PROT_WRITE,
					   flag_map, fd, 0)) == MAP_FAILED)
		exit(EXIT_FAILURE);

	close(fd);

	printf("* mmap file: %s\n", MMAP_FILENAME);

	while (true) {
		printf("\'*\' print current mmap otherwise input text to mmap. >> ");
		if (fgets(a_input, sizeof(a_input), stdin) == NULL)
			exit(EXIT_FAILURE);

		if (a_input[0] == '*') {
			printf("Current mmap -> \'%.*s\'\n", MMAP_SIZE, p_map);
		} else {
			a_input[strlen(a_input) - 1] = 0;
			memcpy(p_map, a_input, strlen(a_input));
			if (msync(p_map, MMAP_SIZE, MS_SYNC) == -1)
				exit(EXIT_FAILURE);
		}
	}

	return 0;
}
