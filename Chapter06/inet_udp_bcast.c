#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
	struct sockaddr_in saddr_s;
	int udp_fd, sockopt, sz_slen;
	char a_sbuf[50];

	if (argc != 2) {
		printf("%s <port> \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&saddr_s, 0x00, sizeof(struct sockaddr_in));

	udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (udp_fd == -1)
		pr_crt("[UDP broadcast] Fail: socket()");

	sockopt = 1;
	socklen_t len_sockopt = sizeof(sockopt);
	if (setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &sockopt, len_sockopt) == -1)
		pr_crt("[UDP broadcast] Fail: setsockopt()");

	saddr_s.sin_family = AF_INET;
	saddr_s.sin_addr.s_addr = INADDR_BROADCAST;
	saddr_s.sin_port = htons((short)atoi(argv[1]));
	sprintf(a_sbuf, "%s", "UDP broadcasting test");

	pr_out("- Send broadcasting data every 3 sec");

	while (true)
	{
		sz_slen = sendto(udp_fd, a_sbuf, strlen(a_sbuf),
				0, (struct sockaddr *) &saddr_s, sizeof(saddr_s));

		if (sz_slen == -1) {
			pr_err("[UDP broadcast] Fail: sendto()");
		} else {
			pr_out("<< send broadcasting message");
		}

		sleep(3);
	}
	
	return 0;
}
