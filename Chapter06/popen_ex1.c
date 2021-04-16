#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *fp_popen;
	char a_buf[1024];
	int n_read;

	if (argc != 2)
		exit(EXIT_FAILURE);

	if ( (fp_popen = popen(argv[1], "r")) == NULL )
		exit(EXIT_FAILURE);

	while (!feof(fp_popen))
	{
		if ((n_read = fread(a_buf, sizeof(char), sizeof(a_buf), fp_popen)) == -1)
			exit(EXIT_FAILURE);

		if (n_read == 0) break;

		printf("[%1$d byte] %2$.*1$s", (int)n_read, a_buf);
	}

	pclose(fp_popen);

	return 0;
}
