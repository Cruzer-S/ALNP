#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define pr_err(arg...) print_msg(stderr, ERR, arg), perror(""), exit(EXIT_FAILURE)
#define pr_out(arg...) print_msg(stdout, REP, arg)

#define LISTEN_BACKLOG 20
#define MAX_POOL 3

int fd_listener;
void start_child(int fd, int idx);

int main(int argc, char *argv[])
{
	int i;
	short port;
	socklen_t len_saddr;
	struct sockaddr_in saddr_s = { } ;
	pid_t pid;
	int wstate;

	if (argc > 2) {
		printf("%s [port number]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc == 2)
		port = (short) atoi((char *) argv[1]);
	else
		port = 0;

	if ((fd_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		pr_err("[TCP server] Fail: socket()");

	saddr_s.sin_family = AF_INET;
	saddr_s.sin_addr.s_addr = INADDR_ANY;
	saddr_s.sin_port = htons(port);

	if (bind(fd_listener, (struct sockaddr *) &saddr_s, sizeof(saddr_s)) == -1)
		pr_err("[TCP server] Fail: bind()");

	if (port == 0) {
		len_saddr = sizeof(saddr_s);
		getsockname(fd_listener, (struct sockaddr *) &saddr_s, &len_saddr);
	}

	pr_out("[TCP Server] Port: #%d", ntohs(saddr_s.sin_port));

	if (listen(fd_listener, LISTEN_BACKLOG) == -1)
		pr_err("[TCP server] Fail: listen()");

	for (i = 0; i < MAX_POOL; i++)
	{
		switch (pid = fork()) {
		case 0:
			start_child(fd_listener, i);
			exit(EXIT_FAILURE);
			break;

		case -1:
			pr_err("TCP server]: Fail: fork()");
			break;

		default:
			pr_out("[TCP server] Making child process No. %d", i);
			break;
		}
	}

	for (i = 0; i < MAX_POOL; i++)
		if (wait(&wstate) == -1)
			pr_err("[TCP server] Fail: wait()");

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
			} else {
				char *find;
				if (!(find = strchr(buf, '\n')))
					break;

				buf[find - buf] = '\0';
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
