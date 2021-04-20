#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/un.h>
#include <arpa/inet.h>

#define PATH_UNIX_SOCKET "/tmp/my_alsp_socket"

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
	int ufd, sz_rlen;
	socklen_t len_saddr;
	struct sockaddr_un saddr_u;
	int flag_recv;
	char a_buf[256];
	ssize_t n_read = 0;
	size_t n_input = 256;
	char *p_input = (char *) malloc(n_input);

	memset(&saddr_u, 0x00, sizeof(saddr_u));
	if (argc != 2) {
		printf("%s <receiver | sender> \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argv[1][0] == 'r')
		flag_recv = 1;
	else
		flag_recv = 0;

	ufd = socket(PF_FILE, SOCK_DGRAM, IPPROTO_IP);
	if (ufd == -1)
		pr_crt("[UNXI_Socket] Fail: socket()");

	saddr_u.sun_family = AF_UNIX;
	snprintf(saddr_u.sun_path, sizeof(saddr_u.sun_path), PATH_UNIX_SOCKET);
	printf("[UNIX Domain] socket path: %s\n", PATH_UNIX_SOCKET);
	if (flag_recv) {
		if (remove(PATH_UNIX_SOCKET)) {
			if (errno != ENOENT)
				pr_crt("[UNIX Socket] Fail: remove()");
		}

		len_saddr = sizeof(saddr_u);
		if (bind(ufd, (struct sockaddr *) &saddr_u, sizeof(saddr_u)) == -1)
			pr_err("[UNIX Socket] Fail: bind()");

		while (true)
		{
			sz_rlen = recvfrom(ufd, a_buf, sizeof(a_buf), 0, NULL, NULL);
			if (sz_rlen == -1)
				pr_err("[UDP Socket] Fail: recvfrom()");

			pr_out("[recv] (%d byte) (%.*s)", sz_rlen, sz_rlen, a_buf);
		}
	} else while (true) {
		if ((n_read = getline(&p_input, &n_input, stdin)) == -1)
			exit(EXIT_FAILURE);

		len_saddr = sizeof(saddr_u);
		sz_rlen = sendto(ufd, p_input, n_read - 1, 0,
				(struct sockaddr *) &saddr_u, len_saddr);
		if (sz_rlen == -1)
			pr_err("[UDP Socket] Fail: sendto()");

		pr_out("[send] (%d byte) (%.*s)", sz_rlen, sz_rlen, p_input);
	}

	return 0;
}
