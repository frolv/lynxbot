#include <regex>
#include "lynxbot.h"
#include "sed.h"
#include "string.h"

int sed(char *s, size_t max, const char *input, const char *sedcmd)
{
	char cmd[MAX_MSG];
	char *t, *regex, *replace;
	int delim, global, ign;

	strncpy(cmd, sedcmd, MAX_MSG);
	if (cmd[0] != 's') {
		_sprintf(s, max, "sed: invalid command '%c'", cmd[0]);
		return 0;
	}
	if (!(delim = cmd[1]) || !*(regex = cmd + 2)) {
		_sprintf(s, max, "sed: unterminated 's' command");
		return 0;
	}
	if (!(t = strchr(regex, delim)) || !*(replace = t + 1)) {
		_sprintf(s, max, "sed: unterminated 's' command");
		return 0;
	}
	*t = '\0';
	if (!(t = strchr(replace, delim))) {
		_sprintf(s, max, "sed: unterminated 's' command");
		return 0;
	}
	*t = '\0';
	for (++t; *t; ++t) {
		switch (*t) {
		case 'g':
			global = 1;
			break;
		case 'i':
			ign = 1;
			break;
		default:
			_sprintf(s, max, "sed: unknown option to 's': '%c'", *t);
			return 0;
		}
	}
	printf("%s\n%s\n%s\n", input, regex, replace);
	return 1;
}
