#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/fcntl.h>

#define print_msg(io, msgtype, arg...) \
			flockfile(io), \
			fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
			fprintf(io, arg), \
			fputc('\n', io), \
			funlockfile(io)

#define pr_crt(arg...) do { print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE); } while (false)
#define pr_err(arg...) do { print_msg(stderr, ERR, arg), perror(""); } while (false)
#define pr_out(arg...) do { print_msg(stdout, REP, arg); } while (false)

#define LISTEN_BACKLOG 256
#define FDSESSION_DEFAULT_TIMEOUT 30

#define ADD_EV(a, b, c) if (add_ev(a, b, c) == -1) pr_crt("fail: add_ev")
#define DEL_EV(a, b) if (del_ev(a, b) == -1) pr_crt("fail: del_ev")

typedef struct fdsession {
	int socketfd, timerfd, timeout;
} FDSESSION;

typedef struct fds_list {
	int n_allocated, n_used;
	FDSESSION *list;
} FDSESSIONLIST;

FDSESSIONLIST fds_list;
const int max_ep_events = 32;
int epollfd;

#define GET_TIME0(a) get_time0(a, sizeof(a)) == NULL ? "error" : a
char *get_time0(char *buf, size_t sz_buf);
int fcntl_setnb(int fd);
int setsockopt_linger(int fd);
int add_ev(int efd, int fd, int timeout);
int del_ev(int efd, int fd);
int get_epollevent(int fd_listener, struct epoll_event *ep_events, int ret_poll);
int fds_new_timerfd(FDSESSION **fds_, int socketfd, int timeout);
int fds_set_timerfd(FDSESSION *fds_);
int fds_deletebytimerfd(int timerfd);
int fds_findbysocketfd(FDSESSION **ret_fds, int fd);
int fds_findbytimerfd(FDSESSION **ret_fds, int fd);

int main(int argc, char *argv[])
{
	int ret, ret_poll, fd_listener;
	char *port;
	struct addrinfo ai, *ai_ret;
	int rc_gai;
	struct epoll_event *ep_events;

	if (argc != 2)
		pr_crt("%s [port]", argv[0]);

	port = strdup(argv[1]);

	ai.ai_family = AF_INET;
	ai.ai_socktype = SOCK_STREAM;
	ai.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;

	if ((rc_gai = getaddrinfo(NULL, port, &ai, &ai_ret)) != 0)
		pr_crt("getaddrinfo() %s", gai_strerror(rc_gai));

	if ((fd_listener = socket(ai_ret->ai_family,
					ai_ret->ai_socktype, ai_ret->ai_protocol)) == -1)
		pr_crt("socket()");

	fcntl_setnb(fd_listener);

	if (bind(fd_listener, ai_ret->ai_addr, ai_ret->ai_addrlen) == -1)
		pr_crt("bind()");

	pr_out("bind: %s", port);

	listen(fd_listener, LISTEN_BACKLOG);
	if ((epollfd = epoll_create(1)) == -1)
		pr_crt("epoll_create");

	if ((ep_events = calloc(max_ep_events, sizeof(struct epoll_event))) == NULL)
		pr_crt("calloc()");

	ADD_EV(epollfd, fd_listener, 0);
	char buf_time[32];

	while (true)
	{
		pr_out("[%s] epoll waiting...", GET_TIME0(buf_time));
		if ((ret_poll = epoll_wait(epollfd, ep_events, max_ep_events, -1)) == -1)
			pr_crt("epoll_wait");

		pr_out("epoll_wait = %d", ret_poll);
		ret = get_epollevent(fd_listener, ep_events, ret_poll);

		if (ret != 0)
			pr_err("get_epollevent()");
	}
	
	return 0;
}

int get_epollevent(int fd_listener, struct epoll_event *ep_events, int ret_poll)
{
	int i, ret, ret_recv, fd;
	char buf[1024], buf_time[32];
	for (i = 0; i < ret_poll; i++)
	{
		if (ep_events[i].events & EPOLLIN) {
			if (ep_events[i].data.fd == fd_listener)
			{
				while (true)
				{
					if ((fd = accept(fd_listener, NULL, NULL)) == -1)
					{
						if (errno == EAGAIN)
							break;

						pr_err("accept()");
						break;
					}

					fcntl_setnb(fd);
					ADD_EV(epollfd, fd, FDSESSION_DEFAULT_TIMEOUT);
					pr_out("accept: add socket = %d", fd);
				}
			} else {
				FDSESSION *ret_fds;
				if (fds_findbytimerfd(&ret_fds, ep_events[i].data.fd) == 0) {
					pr_out("timeout[%s] expired/closed (sfd/tfd=%d/%d)", GET_TIME0(buf_time),
							ret_fds->socketfd, ret_fds->timerfd);

					setsockopt_linger(ret_fds->socketfd);
					DEL_EV(epollfd, ret_fds->socketfd);
				} else {
					if ((ret_recv = recv(ep_events[i].data.fd, buf, sizeof(buf), 0)) == -1) {
						pr_err("recv()");
					} else {
						if (ret_recv == 0) {
							pr_out("closed by foreign host (sfd %d)", ep_events[i].data.fd);
							DEL_EV(epollfd, ep_events[i].data.fd);
						} else {
							pr_out("recv[%s] (fd %d, n %d) = %.*s", GET_TIME0(buf_time),
									ep_events[i].data.fd, ret_recv, ret_recv, buf);
							if ((ret = fds_findbysocketfd(&ret_fds, ep_events[i].data.fd)) != 0)
								return ret;

							if ((ret = fds_set_timerfd(ret_fds)) != 0)
								return ret;
						}
					}
				}
			}
		} else if (ep_events[i].events & EPOLLERR) {
			pr_out("fd(%d) epoll event(%d) err(%s)",
					ep_events[i].data.fd, ep_events[i].events, strerror(errno));
		} else {
			pr_out("fd(%d) epoll event(%d) err(%s)",
					ep_events[i].data.fd, ep_events[i].events, strerror(errno));
		}
	}

	return 0;
}

int add_ev(int efd, int fd, int timeout)
{
	struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1)
		pr_crt("fd(%d) EPOLL_CTL_ADD Error(%d:%s)", fd, errno, strerror(errno));

	if (timeout > 0) {
		int ret;
		FDSESSION *ret_fds;

		if ((ret = fds_new_timerfd(&ret_fds, fd, timeout)) != 0)
			return ret;

		if ((ret = fds_set_timerfd(ret_fds)) != 0)
			return ret;

		ev.events = EPOLLIN;
		ev.data.fd = ret_fds->timerfd;

		if (epoll_ctl(efd, EPOLL_CTL_ADD, ret_fds->timerfd, &ev) == -1)
			pr_crt("fd(%d) EPOLL_CTL_ADD error(%d:%s)", fd, errno, strerror(errno));
	}

	return 0;
}

int del_ev(int efd, int fd)
{
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL) == -1)
		pr_crt("fd(%d) EPOLL_CTL_DEL error(%d:%s)", fd, errno, strerror(errno));

	close(fd);
	
	int ret;
	FDSESSION *ret_fds;

	if ((ret = fds_findbysocketfd(&ret_fds, fd)) != 0)
		return ret;

	if (epoll_ctl(efd, EPOLL_CTL_DEL, ret_fds->timerfd, NULL) == -1)
		pr_crt("fd(%d) EPOLL_CTL_DEL error(%d:%s)", ret_fds->timerfd, errno, strerror(errno));

	close(ret_fds->timerfd);

	if ((ret = fds_deletebytimerfd(ret_fds->timerfd)) != 0)
		return ret;

	return 0;
}

int fds_new_timerfd(FDSESSION **fds, int socketfd, int timeout)
{
	if (fds_list.n_allocated == fds_list.n_used) {
		int n_allocated = fds_list.n_allocated + 16;
		FDSESSION *p_fds = realloc(fds_list.list, n_allocated * sizeof(FDSESSION));

		if (p_fds == NULL) {
			return errno;
		}

		fds_list.list = p_fds;
		fds_list.n_allocated = n_allocated;
	}

	int fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd == -1) {
		perror("timerfd_create");
		return errno;
	}

	fds_list.list[fds_list.n_used].socketfd = socketfd;
	fds_list.list[fds_list.n_used].timerfd = fd;
	fds_list.list[fds_list.n_used].timeout = timeout;

	*fds = &fds_list.list[fds_list.n_used];
	++fds_list.n_used;

	return 0;
}

int fds_set_timerfd(FDSESSION *fds)
{
	struct itimerspec rt_tspec = { .it_value.tv_sec = fds->timeout };

	if (timerfd_settime(fds->timerfd, 0, &rt_tspec, NULL) == -1) {
		pr_err("timerfd_settime");
		return errno;
	}

	char buf_time[32];
	pr_out("timeout[%s] start (sfd/tfd = %d/%d, timeout %d)", GET_TIME0(buf_time),
			fds->socketfd, fds->timerfd, fds->timeout);

	return 0;
}

int fds_deletebytimerfd(int timerfd)
{
	int i, flag = 0;
	FDSESSION *iter_fds;

	for (i = 0; i < fds_list.n_used; i++) {
		iter_fds = &fds_list.list[i];
		if (iter_fds->timerfd == timerfd) {
			if (i != (fds_list.n_used - 1))
				fds_list.list[i] = fds_list.list[fds_list.n_used - 1];

			fds_list.list[fds_list.n_used - 1].socketfd = -1;
			fds_list.list[fds_list.n_used - 1].timerfd = -1;

			flag = 1;
			break;
		}
	}

	if (flag == 0)
		return ENOENT;

	--fds_list.n_used;

	return 0;
}

int fds_findbysocketfd(FDSESSION **ret_fds, int fd)
{
	int i, flag = 0;
	FDSESSION *iter_fds;

	for (i = 0; i < fds_list.n_used; i++) {
		iter_fds = &fds_list.list[i];

		if (iter_fds->socketfd == fd) {
			*ret_fds = iter_fds;
			flag = 1;
			break;
		}
	}

	if (flag == 0)
		return -1;

	return 0;
}

int fds_findbytimerfd(FDSESSION **ret_fds, int fd)
{
	int i, flag = 0;
	FDSESSION *iter_fds;

	for (i = 0; i < fds_list.n_used; i++)
	{
		iter_fds = &fds_list.list[i];
		if (iter_fds->timerfd == fd) {
			*ret_fds = iter_fds;
			flag = 1;
			break;
		}
	}

	if (flag == 0)
		return -1;

	return 0;
}

int fcntl_setnb(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL)) == -1)
		return errno;

	return 0;
}

int setsockopt_linger(int fd)
{
	struct linger so_linger = { .l_onoff = 1, .l_linger = 0 };

	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) == -1)
		return errno;

	return 0;
}

char *get_time0(char *buf, size_t sz_buf)
{
#define STR_TIME_FORMAT "%H:%M:%S"
	struct timespec tspec;
	struct tm tm_now;
	size_t sz_ret;

	if (buf == NULL || sz_buf < 20)
		return NULL;

	if (clock_gettime(CLOCK_MONOTONIC, &tspec) == -1)
		return NULL;

	localtime_r((time_t *) &tspec.tv_sec, &tm_now);
	if ((sz_ret = strftime(buf, sz_buf, STR_TIME_FORMAT, &tm_now)) == 0)
		return NULL;

	return buf;
}
