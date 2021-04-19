#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/un.h>

int main(int argc, char *argv[])
{
	int sfd;
	struct sockaddr_un saddr_un;

	if (argc != 2) {
		printf("Usage: %s <path>", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("UNIX Domain Socket: %s", argv[1]);

	sfd = socket(AF_UNIX, SOCK_STREAM, IPPROTO_IP);
	if (sfd == -1)
		exit(EXIT_FAILURE);

	memset(&saddr_un, 0x00, sizeof(saddr_un));
	saddr_un.sun_family = AF_UNIX;
	snprintf(saddr_un.sun_path, sizeof(saddr_un.sun_path), "%s", argv[1]);
	if (connect(sfd, (struct sockaddr *) &saddr_un, sizeof(saddr_un)) == -1)
		exit(EXIT_FAILURE);

	while (true)
	{
		time_t raw_time = time(NULL);
		char *sbuf = ctime(&raw_time);
		*strchr(sbuf, '\n') = '\0';

		printf("[Send:%zd] (%s)\n", strlen(sbuf), sbuf);
		if (send(sfd, sbuf, strlen(sbuf), 0) == -1)
			exit(EXIT_FAILURE);

		char rbuf[2048];
		recv(sfd, rbuf, sizeof(rbuf), 0);

		printf("[Recv:%zd] %s\n", strlen(rbuf), rbuf);
	}

	return 0;
}
