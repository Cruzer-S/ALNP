#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <netdb.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

int main(int argc, char *argv[])
{
	int fd, rc_gai, flag_once = 0;
	struct addrinfo ai_dest, *ai_dest_ret;
	struct sockaddr_storage sae_local;
	socklen_t len_sae_local = sizeof(sae_local);
	char addrstr[INET6_ADDRSTRLEN], portstr[8];

	if (argc != 4)
		pr_crt("%s <hostname> <port> "
			   "<SO_REUSEADDR off(0) on(non-zero)>", argv[0]);
	
	for (int i = 0; ; i++)
	{
		memset(&ai_dest, 0x00, sizeof(ai_dest));
		ai_dest.ai_family = AF_UNSPEC;
		ai_dest.ai_socktype = SOCK_STREAM;
		ai_dest.ai_flags = AI_ADDRCONFIG;

		if ( (rc_gai = getaddrinfo(argv[1], argv[2], &ai_dest, &ai_dest_ret)) != 0)
			pr_crt("Fail: getaddrinfo() %s", gai_strerror(rc_gai));

		if ( (fd = socket(ai_dest_ret->ai_family,
						ai_dest_ret->ai_socktype,
						ai_dest_ret->ai_protocol)) == -1)
			pr_crt("[Client] Fail: socket()");

		if (argv[3][0] != '0') {
			int sockopt = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) == -1)
				pr_crt("[Client] Fail: setsockopt");

			pr_out("Socket option = SO_REUSEADDR (on)");
		} else {
			pr_out("Socket option = SO_REUSEADDR (off)");
		}

		if (flag_once)
			if (bind(fd, (struct sockaddr *) &sae_local, len_sae_local) == -1)
				pr_crt("Fail: bind()");

		if (connect(fd, ai_dest_ret->ai_addr, ai_dest_ret->ai_addrlen) == -1)
			pr_crt("Fail: connect()");

		if (flag_once == 0) {
			if (getsockname(fd, (struct sockaddr *) &sae_local, &len_sae_local) == -1)
				pr_crt("Fail: getsockname()");

			if ( (rc_gai = getnameinfo((struct sockaddr *) &sae_local, len_sae_local,
							addrstr, sizeof(addrstr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV)) )
					pr_crt("Fail: getnameinfo()");
		}

		flag_once = 1;

		pr_out("Connection established");
		pr_out("\tLocal(%s:%s) => Destination(%s:%s)", addrstr, portstr, argv[1], argv[2]);
		pr_out(">> Press any key to disconnect.");

		(void) getchar();

		close(fd);
		pr_out(">> Press any key to reconnect.");

		(void) getchar();
	}

	return 0;
}
