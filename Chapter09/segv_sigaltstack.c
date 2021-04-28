#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#include <sys/signal.h>
#include <unistd.h>

#define print_msg(io, msgtype, arg...) \
flockfile(io), \
fprintf(io, "[" #msgtype "] [%s/%s:%03d] ", __FILE__, __FUNCTION__, __LINE__), \
fprintf(io, arg), \
fputc('\n', io), \
funlockfile(io)

#define pr_crt(arg...) do { print_msg(stderr, CRT, arg), perror(""), exit(EXIT_FAILURE); } while (false)
#define pr_err(arg...) do { print_msg(stderr, ERR, arg), perror(""); } while (false)
#define pr_out(arg...) do { print_msg(stdout, REP, arg); } while (false)

#define SZ_BUFFER (1024 * 1024)

int exhaust_stack(int count);
void inst_sighandler(void);
void sa_handler_segv(int signum);

int flag_altstack;

#define SZ_SIGHANDLER_STACK 16384

stack_t g_ss;

int main(int argc, char *argv[])
{
	if (argc == 2 && argv[1][0] == '1') {
		pr_out("[Enable] alternate signal stack.");
		flag_altstack = 1;
	} else {
		pr_out("[Disable] alternate signal stack.");
	}

	pr_out("SIGSTKSZ(%d) MINSIGSTKSZ(%d)", SIGSTKSZ, MINSIGSTKSZ);

	inst_sighandler();

	exhaust_stack(100); // overflow

	return 0;
}

int exhaust_stack(int count)
{
	char buffer[SZ_BUFFER];

	if (count <= 0) {
		pr_out(">> stopping recursive func.");
		return 0;
	} else pr_out("[%d] Current stack addr: %p", count, buffer);

	exhaust_stack(count - 1);

	return 0;
}

void sa_handler_segv(int signum)
{
	time_t t_now = time(0);

	struct tm *tm_now = localtime(&t_now);

	snprintf(g_ss.ss_sp, g_ss.ss_size,
			"SEGV: Time(%02d:%02d:%02d)",
			tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

	pr_out("%s", (char *)g_ss.ss_sp);

	fflush(stdout);

	abort();
}

void inst_sighandler(void)
{
	struct sigaction sa_segv;
	memset(&sa_segv, 0x00, sizeof(struct sigaction));

	if ( (g_ss.ss_sp = malloc(SZ_SIGHANDLER_STACK)) == NULL)
		exit(EXIT_FAILURE);

	g_ss.ss_size = SZ_SIGHANDLER_STACK;
	g_ss.ss_flags = 0;

	if (flag_altstack) {
		if (sigaltstack(&g_ss, NULL) == -1)
			exit(EXIT_FAILURE);

		sa_segv.sa_flags = SA_ONSTACK;
	} else {
		sa_segv.sa_flags = 0;
	}

	sa_segv.sa_handler = sa_handler_segv;

	sigaction(SIGSEGV, &sa_segv, 0);
}
