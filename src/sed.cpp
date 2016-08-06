#include <regex>
#include "lynxbot.h"
#include "sed.h"
#include "string.h"

static struct sedinfo {
	const char *regex;
	const char *replace;
	int global;
	int ignore;
	int error;
	int errtok;
}

static int parsecmd(struct sedinfo *s, char *cmd);
static void puterr(char *out, struct sedinfo *s);

int sed(char *s, size_t max, const char *input, const char *sedcmd)
{
	char cmd[MAX_MSG];
	char *t;
	struct sedinfo sedbuf;

	strncpy(cmd, sedcmd, MAX_MSG);
	if (!parsecmd(&sedbuf, cmd)) {
		puterr(out, &sedbuf);
		return 0;
	}

	printf("%s\n%s\n%s\n", input, sedbuf.regex, sedbuf.replace);
	return 1;
}

#define INVCMD 1

static int parsecmd(struct sedinfo *s, char *cmd)
{
	if (cmd[0] != 's') {
		s->error = INVCMD;
		s->errtok = cmd[0];
		return 0;
	}
}

static void puterr(char *out, struct sedinfo *s)
{
	switch (s->error) {
	case INVCMD:
		_sprintf(out, max, "sed: invalid command '%c'", s->errtok);
		break;
	default:
		break;
	}
}
