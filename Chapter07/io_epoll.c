#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netdb.h>
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

#define LISTEN_BACKLOG 256
#define ADD_EV(A, B) if (add_ev(A, B) == -1) { pr_crt("Fail: add_ev"); }
#define DEL_EV(A, B) if (del_ev(A, B) == -1) { pr_crt("Fail: del_ev"); }

int add_ev(int efd, int fd);
int del_ev(int efd, int fd);
int fcntl_setnb(int fd);

const int max_ep_events = 256;
int epoll_fd;

int main(int argc, char *argv[])
{
	socklen_t len_saddr;
	int fd, fd_listener;
	int i, ret_recv, ret_epoll;
	int max_open_files;
	char *port, buf[1024];
	struct epoll_event *ep_events;
	struct addrinfo ai, *ai_ret;
	int rc_gai;

	if (argc != 2)
		pr_crt("%s [port]\n", argv[0]);
	else
		port = strdup(argv[1]);

	if ((max_open_files = sysconf(_SC_OPEN_MAX)) == -1)
		pr_crt("sysconf(_SC_OPEN_MAX) error");

	memset(&ai, 0x00, sizeof(ai));
	ai.ai_family = AF_UNSPEC;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ((rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret)) != 0)
		pr_crt("Fail: getaddrinfo() %s", gai_strerror(rc_gai));

	if ((fd_listener = socket(ai_ret->ai_family, ai_ret->ai_socktype, ai_ret->ai_protocol)) == -1)
		pr_crt("Fail: socket()");

	fcntl_setnb(fd_listener);
	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1)
		pr_crt("Fail: bind()");

	if (listen(fd_listener, LISTEN_BACKLOG) == -1)
		pr_crt("Fail: listen()");

	if ((epoll_fd = epoll_create(1)) == -1)
		pr_crt("Fail: epoll_create()");

	if ((ep_events = calloc(max_ep_events, sizeof(struct epoll_event))) == NULL)
		pr_crt("Fail: calloc()");

	ADD_EV(epoll_fd, fd_listener);

	while (true)
	{
		pr_out("epoll waiting...");
		if ((ret_epoll = epoll_wait(epoll_fd, ep_events, max_open_files, -1)) == -1) {
			pr_err("epoll_wait() error");
			continue;
		}

		pr_out("epoll return: %d", ret_epoll);

		for (i = 0; i < ret_epoll; i++)
		{
			if (ep_events[i].events & EPOLLIN) {
				if (ep_events[i].data.fd == fd_listener) {
					struct sockaddr_storage saddr_c;
					while (true)
					{
						len_saddr = sizeof(saddr_c);
						fd = accept(fd_listener, (struct sockaddr *) &saddr_c, &len_saddr);
						if (fd == -1) {
							if (errno == EAGAIN)
								break;

							pr_err("error get connection from listen socket");
							break;
						}

						fcntl_setnb(fd);
						ADD_EV(epoll_fd, fd);

						pr_out("accept: add socket (%d)", fd);
					}

					continue;
				}

				if ((ret_recv = recv(ep_events[i].data.fd, buf, sizeof(buf), 0)) == -1) {
					DEL_EV(epoll_fd, fd);
					pr_err("Fail: recv");

					continue;
				}

				if (ret_recv == 0) {
					pr_out("fd(%d): Session closed", ep_events[i].data.fd);
					DEL_EV(epoll_fd, ep_events[i].data.fd);
				} else {
					pr_out("recv(fd = %d, n = %d) = %.*s",
							ep_events[i].data.fd, ret_recv, ret_recv, buf);
				}
			} else if (ep_events[i].events & EPOLLPRI) {
				pr_out("EPOLLPRI: Urgent data detected");

				if ((ret_recv = recv(ep_events[i].data.fd, buf, 1, MSG_OOB)) == -1) {
					pr_err("Fail: recv()");
					DEL_EV(epoll_fd, ep_events[i].data.fd);
				}

				pr_out("recv(fd = %d, n = 1) = %.*s (OOB)", ep_events[i].data.fd, 1, buf);
			} else if (ep_events[i].events & EPOLLERR) {
				pr_err("Fail: EPOLLERR");
				DEL_EV(epoll_fd, ep_events[i].data.fd);
			} else {
				pr_out("fd(%d) epoll event (%d) err (%s)",
						ep_events[i].data.fd, ep_events[i].events, strerror(errno));
				DEL_EV(epoll_fd, ep_events[i].data.fd);
			}
		}
	}
	
	return 0;
}

int add_ev(int efd, int fd)
{
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLPRI;
	ev.data.fd = fd;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		pr_out("fd(%d) EPOLL_CTL_ADD Error(%d:%s)", fd, errno, strerror(errno));
		return -1;
	}

	return 0;
}

int del_ev(int efd, int fd)
{
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		pr_out("fd)%d_ EPOLL_CTL_DEL Error(%d:%s)", fd, errno, strerror(errno));
		return -1;
	}

	close(fd);

	return 0;
}

int fcntl_setnb(int fd)
{
	if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1)
		return errno;

	return 0;
}
