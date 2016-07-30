#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define ABT 0
#define REL 1
#define SRC 2

/* full name of the command */
_CMDNAME("about");
/* description of the command */
_CMDDESCR("print bot information");
/* command usage synopsis */
_CMDUSAGE("$about [-s|-r]");

static const char *SOURCE = "https://github.com/frolv/lynxbot";
static const char *RELEASE = "https://github.com/frolv/lynxbot/releases/latest";

/* about: print bot information */
std::string CommandHandler::about(char *out, struct command *c)
{
	int opt, type;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "latest-release", NO_ARG, 'r' },
		{ "source", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	opt_init();
	type = ABT;
	while ((opt = getopt_long(c->argc, c->argv, "rs", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'r':
			type = REL;
			break;
		case 's':
			type = SRC;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	switch (type) {
	case REL:
	case SRC:
		_sprintf(out, MAX_MSG, "[ABOUT] %s",
				type == REL ? RELEASE : SOURCE);
		break;
	default:
		_sprintf(out, MAX_MSG, "[ABOUT] %s is running %s %s. "
				"Find out more at %s", m_name, _BOT_NAME,
				BOT_VER, _BOT_WEBSITE);
		break;
	}
	return "";
}
