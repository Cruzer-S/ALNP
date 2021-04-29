#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)


#define SZ_RECV_BUF 16384
#define SIGRT_LISTEN_TO SIGRTMIN
#define SIGRT_NORM_IO SIGRTMIN + 1

int add_rts_socket(int fd, int i_sig);
int start_sigrt(int fd_listener);
int make_listener(int family, char *port_no);

int main(int argc, char *argv[])
{
	char *port;
	sigset_t sigset_mask;
	int fd_listener;

	if (argc != 2)
		pr_crt("%s [port]", argv[0]);

	port = strdup(argv[1]);

	sigemptyset(&sigset_mask);
	sigaddset(&sigset_mask, SIGRT_LISTEN_TO);
	sigaddset(&sigset_mask, SIGRT_NORM_IO);

	if (sigprocmask(SIG_BLOCK, &sigset_mask, NULL) == -1)
		pr_crt("sigprocmask() error");

	if ( (fd_listener = make_listener(AF_INET, port)) == -1)
		pr_crt("make_listener() error");

	if (start_sigrt(fd_listener) == -1)
		pr_crt("start_sigrt() error");

	return 0;
}

int start_sigrt(int fd_listener)
{
	int fd_client, ret_recv, i_sig;
	char rbuf[SZ_RECV_BUF];
	sigset_t sigset_mask;
	siginfo_t si_rt;

	add_rts_socket(fd_listener, SIGRT_LISTEN_TO);
	sigemptyset(&sigset_mask);
	sigaddset(&sigset_mask, SIGRT_LISTEN_TO);
	sigaddset(&sigset_mask, SIGRT_NORM_IO);

	while (true)
	{
		i_sig = sigwaitinfo(&sigset_mask, &si_rt);
		if (i_sig == SIGRT_LISTEN_TO) {
			struct sockaddr_storage saddr_c;
			socklen_t len_saddr = sizeof(saddr_c);

			fd_client = accept(fd_listener, (struct sockaddr *) &saddr_c, &len_saddr);
			if (fd_client == -1) {
				pr_err("accept() error");
				continue;
			}

			pr_out("[SIGRT] add socket: %d", fd_client);
			add_rts_socket(fd_client, SIGRT_NORM_IO);
		} else if (i_sig == SIGRT_NORM_IO) {
			if (si_rt.si_band & POLLIN) {
				if ((ret_recv = recv(si_rt.si_fd, rbuf, sizeof(rbuf), 0)) == -1) {
					pr_err("fd: %d, recv: %s", si_rt.si_fd, strerror(errno));
				} else {
					if (ret_recv == 0) {
						pr_out("close fd: %d", si_rt.si_fd);
						close(si_rt.si_fd);
					} else {
						pr_out("recv (fd: %d, n: %d) = %.*s",
								si_rt.si_fd, ret_recv, ret_recv, rbuf);
					}
				}
			} else if (si_rt.si_band & POLLERR) {
				pr_err("POLLERR");
			} else if (si_rt.si_band & POLLHUP) {
				pr_err("POLLHUP");
			} else {
				pr_err("Unknown band(%ld)", si_rt.si_band);
			}
		}
	}

	return 0;
}

int make_listener(int family, char *port_no)
{
	struct addrinfo ai, *ai_ret;
	int rc_gai, fd;

	memset(&ai, 0x00, sizeof(struct addrinfo));

	ai.ai_family = family;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ( (rc_gai = getaddrinfo(NULL, port_no, &ai, &ai_ret)) != 0)
		pr_crt("getaddrinfo() error: %s", gai_strerror(rc_gai));

	if ((fd = socket(ai_ret->ai_family, ai_ret->ai_socktype,
					ai_ret->ai_protocol)) == -1)
		return -1;

	if (bind(fd, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1) {
		close(fd);
		return -2;
	}

	if (!strncmp(port_no, "0", strlen(port_no))) {
		struct sockaddr_storage saddr_s;
		socklen_t len_saddr = sizeof(saddr_s);
		getsockname(fd, (struct sockaddr *) &saddr_s, &len_saddr);
		if (saddr_s.ss_family == AF_INET) {
			pr_out("bind: IPv4 Port #%d",
					ntohs(((struct sockaddr_in *)&saddr_s)->sin_port));
		} else if (saddr_s.ss_family == AF_INET6) {
			pr_out("bind: IPv6 Port #%d",
					ntohs(((struct sockaddr_in6 *)&saddr_s)->sin6_port));
		} else {
			pr_out("getsockname: ss_family = %d", saddr_s.ss_family);
		}
	} else {
		pr_out("bind: %s", port_no);
	}

	listen(fd, 256);

	return fd;
}

int add_rts_socket(int fd, int i_sig)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1)
		return -1;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC) == -1)
		return -2;

	if (fcntl(fd, F_SETSIG, i_sig) == -1)
		return -3;

	if (fcntl(fd, F_SETOWN, getpid()) == -1)
		return -4;

	return 0;
}
