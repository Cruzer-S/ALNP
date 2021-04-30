#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/signalfd.h>

#define MAX_POOL 3

#define handle_error(message) \
	do { perror(message); exit(EXIT_FAILURE); } while (false);

int start_child(int sfd, sigset_t sigset);

int main(void)
{
	int sfd_child, sfd_parent;
	struct signalfd_siginfo fdsi;
	sigset_t sigmask_child, sigmask_parent;
	ssize_t ret_read;

	printf("parent: pid %d\n", (int) getpid());

	sigemptyset(&sigmask_child);
	sigaddset(&sigmask_child, SIGUSR1);
	sigaddset(&sigmask_child, SIGCHLD);

	sigemptyset(&sigmask_parent);
	sigaddset(&sigmask_parent, SIGCHLD);

	if (sigprocmask(SIG_BLOCK, &sigmask_child, NULL) == -1)
		handle_error("sigprocmask");

	if (sigprocmask(SIG_BLOCK, &sigmask_parent, NULL) == -1)
		handle_error("sigprocmask");

	if ((sfd_child = signalfd(-1, &sigmask_child, 0)) == -1)
		handle_error("signalfd");

	if ((sfd_parent = signalfd(-1, &sigmask_parent, 0)) == -1)
		handle_error("signalfd");

	for (int i = 0; i < MAX_POOL; i++)
		start_child(sfd_child, sigmask_child);

	while (true)
	{
		ret_read = read(sfd_parent, &fdsi, sizeof(struct signalfd_siginfo));
		if (ret_read != sizeof(struct signalfd_siginfo))
			handle_error("read");

		if (fdsi.ssi_signo == SIGCHLD) {
			int is_refork = 0;
			int optflags = WNOHANG | WEXITED | WSTOPPED | WCONTINUED;
			siginfo_t wsiginfo = { .si_pid = 0 };
			char *str_status;

			while (true)
			{
				if (waitid(P_ALL, 0, &wsiginfo, optflags) == 0 && wsiginfo.si_pid != 0)
				{
					switch (wsiginfo.si_code)
					{
					case CLD_EXITED:
						str_status = "Exited"; is_refork = 1;
						break;

					case CLD_KILLED:
						str_status = "Killed"; is_refork = 1;
						break;

					case CLD_DUMPED:
						str_status = "Dumped"; is_refork = 1;
						break;

					case CLD_STOPPED:
						str_status = "Stopped"; is_refork = 1;
						break;

					case CLD_CONTINUED:
						str_status = "Continued";
						break;

					default:
						str_status = "si_code";
						break;
					}
				}

				printf("Parent: child pid(%d) %s(%d:%d)\n",
						wsiginfo.si_pid, str_status, wsiginfo.si_status, fdsi.ssi_status);

				if (is_refork != 0) {
					is_refork = 0;

					printf("Parent: re-fork child\n");
					start_child(sfd_child, sigmask_child);
				} else {
					break;
				}
			}
		}
	}

	return 0;
}

int start_child(int sfd, sigset_t sigset)
{
	ssize_t ret_read;
	struct signalfd_siginfo fdsi;

	switch (fork())
	{
		case 0:
			printf("\tChild: pid = %d\n", (int) getpid());
			while (true)
			{
				ret_read = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
				if (ret_read != sizeof(struct signalfd_siginfo))
					handle_error("read");

				if (fdsi.ssi_signo == SIGUSR1) {
					printf("\tChild: SIGUSR1 (pid %d)\n", (int) getpid());
				} else if (fdsi.ssi_signo == SIGTERM) {
					printf("\tChild: SIGTERM: call exit(EXIT_SUCCESS)\n");
					exit(EXIT_SUCCESS);
				} else {
					printf("\tChild: Read unexpected signal (%d)\n", fdsi.ssi_signo);
					break;
				}
			}
			
			exit(EXIT_FAILURE);
			break;

		default:
			return 0;
			break;

		case -1:
			handle_error("fork");
			return errno;
			break;
	}

	return 0;
}
