#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>

#define MAX_EXPR_SUB_MATCH 5
#define DEFAULT_REGEX_STR "(</.+>).*<br>"
#define DEFAULT_DEST_STR "<center>align to center</center> "\
						 "align to left <br>New Line<br><br><p>"

int main(int argc, char *argv[])
{
	int i, ret;
	char *p_regex_str;
	char *p_dest_str;

	regex_t re_expr;
	regmatch_t rm_matchtab[MAX_EXPR_SUB_MATCH];
	char errbuf[0xff];

	if (argc != 3) {
		printf("Using default string!!\n");
		printf("* Dest str: %s\n", DEFAULT_DEST_STR);
		p_dest_str = strdup(DEFAULT_DEST_STR);
		p_regex_str = strdup(DEFAULT_REGEX_STR);
	} else {
		p_dest_str = strdup(argv[1]);
		p_regex_str = strdup(argv[2]);
	}

	if ( (ret = regcomp(&re_expr, p_regex_str, REG_EXTENDED | REG_NEWLINE)) ) {
		regerror(ret, &re_expr, errbuf, sizeof(errbuf));
		printf("Error regcomp(): %s\n", errbuf);
		exit(EXIT_FAILURE);
	}

	printf("regcop: %s\n", p_regex_str);
	memset(rm_matchtab, 0x00, sizeof(rm_matchtab));
	if (regexec(&re_expr, p_dest_str, MAX_EXPR_SUB_MATCH, rm_matchtab, 0)) {
		printf("fail to match\n");
	} else {
		printf("* All match offset: (%d -> %d), len(%d): %.*s\n",
				rm_matchtab[0].rm_so, rm_matchtab[0].rm_eo,
				rm_matchtab[0].rm_eo - rm_matchtab[0].rm_so,
				rm_matchtab[0].rm_eo - rm_matchtab[0].rm_so,
				&p_dest_str[rm_matchtab[0].rm_so]);

		for (i = 1; i < MAX_EXPR_SUB_MATCH; i++) {
			if (rm_matchtab[i].rm_so == -1) break;

			printf("* Submatch[%d] offset: (%d -> %d), len(%d): %.*s\n", i,
				rm_matchtab[i].rm_so, rm_matchtab[i].rm_eo,
				rm_matchtab[i].rm_eo - rm_matchtab[i].rm_so,
				rm_matchtab[i].rm_eo - rm_matchtab[i].rm_so,
				&p_dest_str[rm_matchtab[i].rm_so]);

		}
	}

	return 0;
}
