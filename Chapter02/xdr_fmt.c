#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	char *p_buf;
	if ( (p_buf = (char *) malloc(sizeof(char) * 65536)) == NULL)
		return EXIT_FAILURE;

	memcpy(p_buf, "1234567890abc", 13);
	p_buf += 13;
	*((long *) p_buf) = 123456;

	return EXIT_SUCCESS;
}
