#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
	struct addrinfo ai, *ai_ret, *ai_cur;
	int i, rc_gai, ni_flag = NI_NUMERICHOST | NI_NUMERICSERV;
	char *addr, *port;
	char addrstr[INET6_ADDRSTRLEN], portstr[8];

	if (argc != 3) {
		printf("%s <address | null> <port | null>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (!strncmp(argv[1], "null", 4))
		addr = NULL;
	else
		addr = strdup(argv[1]);

	if (!strncmp(argv[2], "null", 4))
		port = NULL;
	else
		port = strdup(argv[2]);

	memset(&ai, 0x00, sizeof(ai));
	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG;

	if (addr == NULL)
		ai.ai_flags |= AI_PASSIVE | AI_V4MAPPED;
	else
		ai.ai_flags |= AI_CANONNAME;

	if ( (rc_gai = getaddrinfo(addr, port, &ai, &ai_ret)) != 0) {
		fprintf(stderr, "Fail: getaddrinfo(): %s\n", gai_strerror(rc_gai));
		exit(EXIT_FAILURE);
	}

	for (i = 0, ai_cur = ai_ret; ai_cur != NULL; ai_cur = ai_cur->ai_next)
	{
		printf("[idx %d] (family %d) (socktype %d) (protocol %d)\n", i,
				ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
		rc_gai = getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
				addrstr, sizeof(addrstr), portstr, sizeof(portstr), ni_flag);

		if (rc_gai) {
			fprintf(stderr, "Fail: getnameinfo(): %s\n", gai_strerror(rc_gai));
			exit(EXIT_FAILURE);
		}

		if (ai_cur->ai_family == AF_INET) {
			printf("\t(IPv4 %s) (addrlen %d)\n", addrstr, ai_cur->ai_addrlen);
			printf("\t(port %s)\n", portstr);

			if (ai_cur->ai_flags & AI_CANONNAME)
				printf("\t(canonname %s)\n", ai_cur->ai_canonname);
		} else if (ai_cur->ai_family == AF_INET6) {
			printf("\t(IPv6 %s) (addrlen %d) (scope_id %d)\n",
					addrstr, ai_cur->ai_addrlen,
					((struct sockaddr_in6 *)ai_cur->ai_addr)->sin6_scope_id);
			printf("\t(port %s)\n", portstr);

			if (ai_cur->ai_flags & AI_CANONNAME)
				printf("\t(canonname %s)\n", ai_cur->ai_canonname);

			struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ai_cur->ai_addr;
			printf("IN6_IS_ADDR_LOOPBACK: %d\n", IN6_IS_ADDR_LOOPBACK(&sa6->sin6_addr));
			printf("IN6_IS_ADDR_LINKLOCAL: %d\n", IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr));
			printf("IN6_IS_ADDR_V4MAPPED: %d\n", IN6_IS_ADDR_V4MAPPED(&sa6->sin6_addr));
		} else {
			printf("ai_family (%d)\n", ai_cur->ai_family);
		}
	}

	return 0;
}
