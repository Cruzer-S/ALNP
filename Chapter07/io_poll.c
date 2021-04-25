#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <netdb.h>

#define LISTEN_BACKLOG 256
#define MAX_FD_SOCKET 0xFF

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

struct pollfd pollfds[MAX_FD_SOCKET];
int cnt_fd_socket;

int add_socket(int fd);
int del_socket(int fd);

int main(int argc, char *argv[])
{
	socklen_t len_saddr;
	int i, fd, fd_listener, ret_recv, ret_ionread, ret_poll;
	char *port, buf[1024];

	if (argc != 2)
		pr_crt("%s [port]\n", argv[0]);
	else
		port = strdup(argv[1]);

	struct addrinfo ai, *ai_ret;
	int rc_gai;
	
	memset(&ai, 0x00, sizeof(ai));
	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ((rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret)) != 0)
		pr_crt("Fail: getaddrinfo %s", gai_strerror(rc_gai));

	if ((fd_listener = socket(ai_ret->ai_family,
					ai_ret->ai_socktype, ai_ret->ai_protocol)) == -1)
		pr_crt("socket() error");

	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1)
		pr_crt("Fail: bind()");

	if (!strncmp(port, "0", strlen(port))) {
		struct sockaddr_storage saddr_s;
		len_saddr = sizeof(saddr_s);

		getsockname(fd_listener, (struct sockaddr *) &saddr_s, &len_saddr);
		if (saddr_s.ss_family == AF_INET)
			pr_out("bind: IPv4 Port #%d",
					ntohs(((struct sockaddr_in *)&saddr_s)->sin_port));
		else if (saddr_s.ss_family == AF_INET6)
			pr_out("bind: IPv6 Port #%d",
					ntohs(((struct sockaddr_in6 *) &saddr_s)->sin6_port));
		else
			pr_out("getsockname: ss_family = %d", saddr_s.ss_family);
	} else {
		pr_out("bind: %s", port);
	}

	listen(fd_listener, LISTEN_BACKLOG);
	add_socket(fd_listener);

	while (true)
	{
		if ((ret_poll = poll(pollfds, cnt_fd_socket, -1)) == -1)
			pr_crt("poll() error");

		pr_out("\tpoll = %d", ret_poll);
		if (pollfds[0].revents & POLLIN) {
			struct sockaddr_storage saddr_c;
			len_saddr = sizeof(saddr_c);

			if ((fd = accept(pollfds[0].fd, (struct sockaddr *) &saddr_c, &len_saddr)) == -1) {
				pr_err("error get connection from listen socket");
				continue;
			}

			if (add_socket(fd) == -1) {
				pr_err("add_socket() error");
				close(fd);
				continue;
			}

			pr_out("accept: add socket(%d)", fd);
			
			continue;
		}

		for (i = 1; i < cnt_fd_socket && ret_poll > 0; i++) {
			if (pollfds[i].revents & POLLIN) {
				pr_out("POLLIN: normal-inband");
				if ((ret_recv = recv(pollfds[i].fd, buf, sizeof(buf), 0)) == -1) {
					pr_err("fd(%d) recv() error %s", pollfds[i].fd, strerror(errno));
				} else {
					if (ret_recv == 0) {
						pr_out("fd(%d) Session closed", pollfds[i].fd);
						del_socket(pollfds[i].fd);
						i--;
					} else {
						pr_out("recv(fd = %d, n = %d) %.*s", pollfds[i].fd, ret_recv, ret_recv, buf);
					}
				}

				ret_poll--;
			} else if (pollfds[i].revents & POLLERR) {
				ret_poll--;
			} else if (pollfds[i].revents & POLLNVAL) {
				ret_poll--;
			} else {
				pr_out("> No signal: fd(%d)", pollfds[i].fd);
			}
		}
	}
		
	return 0;
}

int add_socket(int fd)
{
	if (cnt_fd_socket < MAX_FD_SOCKET) {
		pollfds[cnt_fd_socket].fd = fd;
		pollfds[cnt_fd_socket].events = POLLIN;

		return ++cnt_fd_socket;
	} else {
		return -1;
	}
}

int del_socket(int fd)
{
	int i, flag = 0;

	close(fd);

	for (i = 0; i < cnt_fd_socket; i++)
	{
		if (pollfds[i].fd == fd) {
			if (i != (cnt_fd_socket - 1)) {
				pollfds[i] = pollfds[cnt_fd_socket - 1];
			}

			pollfds[cnt_fd_socket].fd = -1;
			flag = -1;
			break;
		}
	}

	if (flag == 0)
		return -1;

	--cnt_fd_socket;

	return i;
}
