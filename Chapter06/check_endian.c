#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

union byte_long {
	uint32_t l;
	uint8_t c[4];
};

int main(void)
{
	union byte_long bl;
	bl.l = 1200000L;

	for (int i = 0; i < 4; i++)
		printf("%02x", bl.c[i]);
	putchar('\n');

	bl.l = htonl(bl.l);

	for (int i = 0; i < 4; i++)
		printf("%02x", bl.c[i]);
	putchar('\n');

	return 0;
}
