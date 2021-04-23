#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
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
	int fd, rc_gai;
	struct addrinfo ai, *ai_ret;

	if (argc != 3)
		pr_crt("%s <hostname> <port>\n", argv[0]);

	memset(&ai, 0x00, sizeof(ai));
	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG;

	if ( (rc_gai = getaddrinfo(argv[1], argv[2], &ai, &ai_ret)) != 0)
		pr_crt("Fail: getaddrinfo() %s", gai_strerror(rc_gai));

	if ( (fd = socket(ai_ret->ai_family, ai_ret->ai_socktype, ai_ret->ai_protocol)) == -1)
		pr_crt("[Client] Fail: socket()");

	(void) connect(fd, ai_ret->ai_addr, ai_ret->ai_addrlen);
	if (errno != EINPROGRESS)
		pr_crt("[Client] Fail: connect()");

	fd_set fdset_w;
	FD_ZERO(&fdset_w);
	FD_SET(fd, &fdset_w);
	if (select(fd + 1, NULL, &fdset_w, NULL, NULL) == -1)
		pr_crt("[Client] Fail: select()");

	int sockopt;
	socklen_t len_sockopt = sizeof(sockopt);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &sockopt, &len_sockopt) == -1)
		pr_crt("[Client] Fail: getsockopt()");

	if (sockopt)
		pr_crt("SO_ERROR: %s (%d)", strerror(sockopt), sockopt);

	close(fd);

	return 0;
}
