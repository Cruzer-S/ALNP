#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/select.h>
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

#define LISTEN_BACKLOG 256
#define MAX_FD_SOCKET 0xFF
#define MAX(A, B) ((A) > (B) ? (A) : (B))

int fd_socket[MAX_FD_SOCKET];
int cnt_fd_socket;

fd_set fds_read;
int fd_biggest;
int add_socket(int fd);
int del_socket(int fd);
int mk_fds(fd_set *fds, int *a_fd_socket);

int main(int argc, char *argv[])
{
	socklen_t len_saddr;
	int fd, fd_listener, ret_recv, ret_select, i;
	char *port, buf[1024];

	for (i = 0; i < MAX_FD_SOCKET; i++)
		fd_socket[i] = -1;

	if (argc != 2)
		pr_crt("%s [port number]", argv[0]);
	else
		port = strdup(argv[1]);

	struct addrinfo ai, *ai_ret;
	int rc_gai;

	memset(&ai, 0, sizeof(ai));
	ai.ai_family = AF_INET;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ( (rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret)) != 0)
		pr_crt("Fail: getaddrinfo() %s", gai_strerror(rc_gai));

	if ( (fd_listener = socket(ai_ret->ai_family,
					ai_ret->ai_socktype, ai_ret->ai_protocol)) == -1)
			pr_crt("Fail: socket()");

	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1)
		pr_crt("Fail: bind()");

	if (!strncmp(port, "0", strlen(port))) {
		struct sockaddr_storage saddr_s;
		len_saddr = sizeof(saddr_s);
		getsockname(fd_listener, (struct sockaddr *) &saddr_s, &len_saddr);
		if (saddr_s.ss_family == AF_INET)
			pr_out("bind: IPv4 Port: #%d", ntohs(((struct sockaddr_in *)&saddr_s)->sin_port));
		else if (saddr_s.ss_family == AF_INET6)
			pr_out("bind: IPv6 Port: #%d",
					ntohs(((struct sockaddr_in6 *) &saddr_s)->sin6_port));
	} else {
		pr_out("bind: %s", port);
	}

	listen(fd_listener, LISTEN_BACKLOG);

	add_socket(fd_listener);

	while (true)
	{
		fd_biggest = mk_fds(&fds_read, fd_socket);
		if ((ret_select = select(fd_biggest + 1, &fds_read, NULL, NULL, NULL)) == -1)
			pr_crt("select() error");

		if (FD_ISSET(fd_listener, &fds_read)) {
			struct sockaddr_storage saddr_c;

			len_saddr = sizeof(saddr_c);
			fd = accept(fd_listener, (struct sockaddr *) &saddr_c, &len_saddr);
			if (fd == -1)
				continue;

			if (add_socket(fd) == -1) {
				close(fd);
				continue;
			}

			pr_out("accept: add socket (%d)", fd);
			continue;
		}

		for (i = 1; i < cnt_fd_socket; i++)
		{
			if (FD_ISSET(fd_socket[i], &fds_read)) {
				pr_out("FD_ISSET: normal-inband");
				if ((ret_recv = recv(fd_socket[i], buf, sizeof(buf), 0)) == -1) {
					pr_err("fd(%d) recv() error (%s)", fd_socket[i], strerror(errno));
				} else {
					if (ret_recv == 0) {
						pr_out("fd(%d): Session closed", fd_socket[i]);
						del_socket(fd_socket[i]);
					} else {
						pr_out("recv(fd=%d, n=%d) = %.*s",
								fd_socket[i], ret_recv, ret_recv, buf);
					}
				}
			}
		}
	}

	return 0;
}

int add_socket(int fd)
{
	if (cnt_fd_socket < MAX_FD_SOCKET) {
		fd_socket[cnt_fd_socket] = fd;
		return ++cnt_fd_socket;
	} else {
		return -1;
	}
}

int del_socket(int fd)
{
	int i, flag;

	flag = 0;

	close(fd);

	for (i = 0; i < cnt_fd_socket; i++) {
		if (fd_socket[i] == fd) {
			if (i != (cnt_fd_socket - 1))
				fd_socket[i] = fd_socket[cnt_fd_socket - 1];

			fd_socket[cnt_fd_socket - 1] = -1;
			flag = 1;
			break;
		}
	}

	if (flag == 0)
		return -1;

	--cnt_fd_socket;

	return i;
}

int mk_fds(fd_set *fds, int *a_fd_socket)
{
	int i, fd_max;

	FD_ZERO(fds);
	for (i = 0, fd_max = -1; i < cnt_fd_socket; i++) {
		fd_max = MAX(fd_max, a_fd_socket[i]);
		FD_SET(a_fd_socket[i], fds);
	}

	return fd_max;
}
