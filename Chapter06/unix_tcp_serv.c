#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_crt(arg...) print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE)
#define pr_err(arg...) print_msg(stderr, ERR, arg), perror("")
#define pr_out(arg...) print_msg(stdout, REP, arg)

#define MAX_POOL 3
#define PATH_UNIX_SOCKET "/tmp/my_socket"

int fd_listener;
void start_child(int fd, int idx);

int main(int argc, char *argv[])
{
	int i;
	pid_t pid;
	struct sockaddr_un saddr_u;

	if ((fd_listener = socket(AF_UNIX, SOCK_STREAM, IPPROTO_IP)) == -1)
		pr_crt("[TCP server] Fail: socket()");

	if (remove(PATH_UNIX_SOCKET))
		if (errno != ENOENT)
			pr_crt("[TCP Socket] Fail: remove()");

	memset(&saddr_u, 0x00, sizeof(saddr_u));
	saddr_u.sun_family = AF_UNIX;
	snprintf(saddr_u.sun_path, sizeof(saddr_u.sun_path), "%s", PATH_UNIX_SOCKET);
	if (bind(fd_listener, (struct sockaddr *) &saddr_u, sizeof(saddr_u)) == -1)
		pr_crt("[TCP server] Fail: bind()");

	pr_out("[UNIX Domain] PATH: #%s", PATH_UNIX_SOCKET);
	listen(fd_listener, 10);

	for (i = 0; i < MAX_POOL; i++)
	{
		switch (pid = fork()) {
		case 0:
			start_child(fd_listener, i);
			break;

		case -1:
			pr_crt("[TCP server] Fail: fork()");
			break;

		default:
			pr_out("[TCP server] Making child process No.%d", i);
			break;
		}
	}

	while (true)
		pause();

	return 0;
}

void start_child(int sfd, int idx)
{
	int cfd, ret_len;
	char buf[40];
	socklen_t len_saddr;
	struct sockaddr_storage saddr_c;

	while (true)
	{
		len_saddr = sizeof(saddr_c);
		cfd = accept(sfd, (struct sockaddr *) &saddr_c, &len_saddr);
		if (cfd == -1) {
			pr_err("[Child] Fail: accept()");
			close(cfd);
			continue;
		}

		if (saddr_c.ss_family == AF_INET)
			pr_out("[Child:%d] accept (ip:port) (%s:%d)", idx,
					inet_ntoa( ((struct sockaddr_in *)&saddr_c)->sin_addr ),
					ntohs( ((struct sockaddr_in *) &saddr_c)->sin_port ));
		else if(saddr_c.ss_family == AF_UNIX)
			pr_out("[Child:%d] accept (UNIX Domain)", idx);

		while (true)
		{
			ret_len = recv(cfd, buf, sizeof(buf), 0);
			if (ret_len == -1) {
				if (errno == EINTR) continue;
				pr_err("[Child:%d] Fail: recv(): %s", idx, strerror(errno));
				break;
			}

			if (ret_len == 0) {
				pr_err("[Child:%d] Session closed", idx);
				close(cfd);
				break;
			}

			pr_out("[Child:%d] RECV(%.*s)", idx, ret_len, buf);
			if (send(cfd, buf, ret_len, 0) == -1) {
				pr_err("[Child:%d] Fail: send() to socket(%d)", idx, cfd);
				close(cfd);
			}

			sleep(1);
		}
	}

	return ;
}
