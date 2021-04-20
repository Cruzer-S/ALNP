#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#define LISTEN_BACKLOG 512
#define MAX_POOL 3

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

int fd_listener;
void start_child(int fd, int idx);

int main(int argc, char *argv[])
{
	int i;
	char *port;
	socklen_t len_saddr;
	pid_t pid;

	if (argc > 2)
		pr_crt("%s [port number]\n", argv[0]);

	if (argc == 2)
		port = strdup(argv[1]);
	else 
		port = strdup("0");
	
	struct addrinfo ai, *ai_ret;
	int rc_gai;

	memset(&ai, 0x00, sizeof(ai));
	ai.ai_family = AF_INET6;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ((rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret)) == -1)
		pr_crt("Fail: getaddrinfo() %s", gai_strerror(rc_gai));

	if ( (fd_listener = socket(
					ai_ret->ai_family,
					ai_ret->ai_socktype,
					ai_ret->ai_protocol)) == -1)
		pr_crt("[TCP server] Fail: socket()");

	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1)
		pr_crt("[TCP server] Fail: bind()");

	if (!strncmp(port, "0", strlen(port))) {
		struct sockaddr_storage saddr_s;
		len_saddr = sizeof(saddr_s);

		getsockname(fd_listener, (struct sockaddr *) &saddr_s,
				&len_saddr);
		if (saddr_s.ss_family == AF_INET) {
			pr_out("[TCP server] IPv4 Port: #%d",
					ntohs(((struct sockaddr_in *) &saddr_s)->sin_port));
		} else if (saddr_s.ss_family == AF_INET6) {
			pr_out("[TCP server] IPv6 Port: #%d",
					ntohs(((struct sockaddr_in *) &saddr_s)->sin_port));
		} else pr_out("[TCP server] (ss_family %d)", saddr_s.ss_family);
	}

	listen(fd_listener, LISTEN_BACKLOG);

	for (i = 0; i < MAX_POOL; i++)
	{
		switch (pid = fork()) {
		case 0:
			start_child(fd_listener, i);
			exit(EXIT_SUCCESS);
			break;

		case 1:
			pr_err("[TCP server] Fail: fork()");
			break;

		default:
			pr_out("[TCP server] Making child process No.%d", i);
			break;
		}
	}

	while (true) pause();

	return 0;
}

void start_child(int sfd, int idx)
{
	int cfd, ret_len, rc_gai;
	socklen_t len_saddr;
	char buf[40], addrstr[INET6_ADDRSTRLEN], portstr[8];
	struct sockaddr_storage saddr_c;

	while (true)
	{
		len_saddr = sizeof(saddr_c);
		if ( (cfd = accept(sfd, (struct sockaddr *) &saddr_c, &len_saddr)) == -1) {
			pr_err("[Child] Fail: accept()");
			continue;
		}

		rc_gai = getnameinfo((struct sockaddr *) &saddr_c, len_saddr,
				addrstr, sizeof(addrstr), portstr, sizeof(portstr), 
				NI_NUMERICHOST | NI_NUMERICSERV);

		if (rc_gai) {
			pr_err("Fail: getnameinfo() %s", gai_strerror(rc_gai));
			continue;
		}

		if (saddr_c.ss_family == AF_INET) {
			pr_out("[Child %d] accept IPv4 (ip:port) %s:%s", idx,
					addrstr, portstr);
		} else if (saddr_c.ss_family == AF_INET) {
			pr_out("[Child %d] accept IPv6 (ip:port,scope) %s:%s,%d", idx,
					addrstr, portstr, 
					((struct sockaddr_in6 *) &saddr_c)->sin6_scope_id);
		}

		while (true)
		{
			ret_len = recv(cfd, buf, sizeof(buf), 0);
			if (ret_len == -1) {
				if(errno == EINTR)
					continue;

				pr_err("[Child %d] Fail: recv() %s", idx, strerror(errno));
				break;
			}

			pr_out("[Child %d] RECV(%.*s)", idx, ret_len, buf);
			if (send(cfd, buf, ret_len, 0) == -1) {
				pr_err("[Child %d] Fail: send() to socket(%d)", idx, cfd);
				break;
			}
		}

		close(cfd);
	}
}
