#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	FILE *fp_popen;
	int i;
	int rc_getline;
	int rc_write;
	char *p_buf;
	size_t len_buf;

	if ( (fp_popen = popen("sort", "w")) == NULL )
		exit(EXIT_FAILURE);

	printf("* waiting for your input: \n");
	for (i = 0; i < 5; i++) {
		if ( (rc_getline = getline(&p_buf, &len_buf, stdin)) == -1)
			exit(EXIT_FAILURE);

		if ( (rc_write = fwrite(p_buf, sizeof(char), rc_getline, fp_popen)) == -1)
			exit(EXIT_FAILURE);

		if (rc_write == 0) break;

		free(p_buf);
		p_buf = NULL;
	}

	printf("* Sorting data -> \n");
	pclose(fp_popen);

	return 0;
}
