#include "command.h"
#include <string.h>

/* argvcat: concatenate argv from start to argc into buf */
void argvcat(char *buf, int argc, char **argv, int start, int space)
{
	*buf = '\0';
	for (; start < argc; ++start) {
		strcat(buf, argv[start]);
		if (space && start != argc - 1)
			strcat(buf, " ");
	}
}
