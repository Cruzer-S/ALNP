#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <arpa/inet.h>

#define print_msg(io, msgtype, arg...) \
	flockfile(io), \
	fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
	fprintf(io, arg), \
	fputc('\n', io), \
	funlockfile(io)

#define print_err

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

	if (argc > 2) {
		printf("%s [port number]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc == 2)
		port = (short) atoi((char *) argv[1]);
	else
		port = 0;

	if ((fd_listener - socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
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

	return 0;
}

void start_child(int sfd, int idx)
{
	return ;
}
