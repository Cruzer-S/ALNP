#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <sys/signal.h>

int num_steps = 200000000;
struct timespec diff_timespec(struct timespec t1, struct timespec t2);

int main(void)
{
	int i, ret;
	struct timespec ts1, ts2, ts_diff;

#ifdef _POSIX_CPUTIME
	clockid_t clock_cpu;
	if ((ret = clock_getcpuclockid(0, &clock_cpu)) != 0)
		perror("error: clock_getcpuclockid()");

	clock_gettime(clock_cpu, &ts1);
#endif

	double x, step, sum = 0.0;
	step = 1.0 / (double) num_steps;
	for (i = 0; i < num_steps; i++) {
		x = (i + 0.5) * step;
		sum += 4.0 / (1.0 + x * x);
	}

	printf("pi = %.8f (sum = %.8f)\n", step * sum, sum);
	sleep(1);

#ifdef _POSIX_CPUTIME
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
	ts_diff = diff_timespec(ts1, ts2);
	printf("elapsed cpu time = %ld.%09ld\n", ts_diff.tv_sec, ts_diff.tv_nsec);
#endif

	return 0;
}

struct timespec diff_timespec(struct timespec t1, struct timespec t2)
{
	struct timespec t;
	t.tv_sec = t2.tv_sec - t1.tv_sec;
	t.tv_nsec = t2.tv_nsec - t1.tv_sec;

	if (t.tv_nsec < 0) {
		t.tv_sec--;
		t.tv_nsec += 1000000000;
	}

	return t;
}
