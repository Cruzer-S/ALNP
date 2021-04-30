#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/eventfd.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (false);

int main(int argc, char *argv[])
{
	int efd, i;
	uint64_t u;
	ssize_t s;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <num> ...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	efd = eventfd(0, 0);
	if (efd == -1)
		handle_error("eventfd");

	switch (fork()) {
	case 0:
		for (i = 0; i < argc; i++) {
			sleep(1);

			printf("\tchild writing %s to efd\n", argv[i]);
			u = strtoull(argv[i], NULL, 0);
			s = write(efd, &u, sizeof(uint64_t));

			if (s != sizeof(uint64_t))
				handle_error("write");
		}

		printf("child completed write loop\n");
		exit(EXIT_SUCCESS);
		break;

	default:
		while (true) {
			s = read(efd, &u, sizeof(uint64_t));
			if (s != sizeof(uint64_t))
				handle_error("read");

			printf("parent read %llu (0x%llx) from efd\n",
					(unsigned long long) u, (unsigned long long) u);
		}

		exit(EXIT_SUCCESS);
		break;
	
	case -1:
		handle_error("fork");
		break;
	}

	return 0;
}
