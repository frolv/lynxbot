#include <regex>
#include <string.h>
#include "lynxbot.h"
#include "sed.h"
#include "stringparse.h"

struct sedinfo {
	char *regex;
	char *replace;
	int global;
	int ignore;
	int error;
	int errtok;
};

static int parsecmd(struct sedinfo *s, char *cmd);
static void puterr(char *out, size_t max, struct sedinfo *s);
static char *readbrackets(char *s);

/* sed: perform a basic sed substitution command on input */
int sed(char *s, size_t max, const char *input, const char *sedcmd)
{
	char cmd[MAX_MSG];
	struct sedinfo sedbuf;
	std::regex pattern;
	auto type = std::regex_constants::format_sed;

	strncpy(cmd, sedcmd, MAX_MSG);
	if (!parsecmd(&sedbuf, cmd)) {
		puterr(s, max, &sedbuf);
		return 0;
	}

	try {
		if (!sedbuf.ignore)
			pattern = std::regex(sedbuf.regex);
		else
			pattern = std::regex(sedbuf.regex, std::regex::icase);
	} catch (std::regex_error) {
		_sprintf(s, max, "sed: invalid regex");
		return 0;
	}

	if (!sedbuf.global)
		type |= std::regex_constants::format_first_only;
	_sprintf(s, max, "%s", std::regex_replace(input, pattern,
				sedbuf.replace, type).c_str());
	return 1;
}

#define INVCMD 1
#define UNTERM 2
#define BADOPT 3

/* parsecmd: break up the sed command into its parts */
static int parsecmd(struct sedinfo *s, char *cmd)
{
	char *t;
	int delim;

	s->global = s->ignore = 0;
	if (cmd[0] != 's') {
		s->error = INVCMD;
		s->errtok = cmd[0];
		return 0;
	}
	if (!(delim = cmd[1]) || !*(s->regex = cmd + 2)) {
		s->error = UNTERM;
		return 0;
	}
	t = s->regex;
	for (t = s->regex; *t; ++t) {
		if (*t == '(' || *t == '[') {
			if (!(t = readbrackets(t))) {
				s->error = UNTERM;
				return 0;
			}
		} else if (*t == delim) {
			if (*(t - 1) != '\\')
				break;
			shift_l(t - 1);
			--t;
		}
	}
	if (!t) {
		s->error = UNTERM;
		return 0;
	}
	*t++ = '\0';
	if (!*(s->replace = t)) {
		s->error = UNTERM;
		return 0;
	}
	while ((t = strchr(t, delim)) && *(t - 1) == '\\')
		shift_l(t - 1);
	if (!t) {
		s->error = UNTERM;
		return 0;
	}
	*t++ = '\0';
	for (; *t; ++t) {
		switch (*t) {
		case 'g':
			s->global = 1;
			break;
		case 'i':
			s->ignore = 1;
			break;
		default:
			s->error = BADOPT;
			s->errtok = *t;
			return 0;
		}
	}

	return 1;
}

/* puterr: write error string corresponding to s->error to out */
static void puterr(char *out, size_t max, struct sedinfo *s)
{
	switch (s->error) {
	case INVCMD:
		_sprintf(out, max, "sed: invalid command '%c'", s->errtok);
		break;
	case UNTERM:
		_sprintf(out, max, "sed: unterminated 's' command");
		break;
	case BADOPT:
		_sprintf(out, max, "sed: unkown option to 's' command "
				"-- '%c'", s->errtok);
		break;
	default:
		break;
	}
}

/* readbrackets: read over a bracketed block */
static char *readbrackets(char *s)
{
	int end;

	end = *s == '(' ? ')' : ']';
	for (++s; *s && *s != end; ++s) {
		if (*s == '\\' && s[1] == end)
			shift_l(s);
	}
	return *s ? s : NULL;
}
