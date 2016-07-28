#include <string.h>
#include "cmdparse.h"
#include "lynxbot.h"

#define ERR_LEN 256

static char err[ERR_LEN];

/* parse_cmd: split cmdstr into argv of c */
int parse_cmd(char *cmdstr, struct CommandHandler::command *c)
{
	int inquotes, type, i;
	char *s;
	static const char *esc = " \\'\"";

	c->argc = inquotes = 0;
	for (s = cmdstr; *s; ++s) {
		switch (*s) {
		case '\\':
			if (!s[1]) {
				_sprintf(err, ERR_LEN, "error: unexpected EOF");
				return 0;
			}
			if (!strchr(esc, s[1])) {
				_sprintf(err, ERR_LEN, "error: unrecognized "
						"escape sequence \\%c", s[1]);
				return 0;
			}
			break;
		case '\'':
		case '"':
			type = *s;
			break;
		case ' ':
			c->argc++;
			*s = '\0';
			break;
		default:
			break;
		}
	}
	c->argc++;

	/* c++ requires malloc to be cast */
	c->argv = (char **)malloc((c->argc + 1) * sizeof(*(c->argv)));

	s = cmdstr;
	for (i = 0; i < c->argc; ++i) {
		c->argv[i] = s;
		s = strchr(s, '\0') + 1;
	}
	c->argv[i] = NULL;
	err[0] = '\0';
	return 1;
}

int free_cmd(struct CommandHandler::command *c)
{
	free(c->argv);
}

char *cmderr()
{
	return err;
}
