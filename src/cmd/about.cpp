#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define ABT 0
#define REL 1
#define SRC 2

/* full name of the command */
CMDNAME("about");
_CMDNAME("about");
/* description of the command */
CMDDESCR("print bot information");
_CMDDESCR("print bot information");
/* command usage synopsis */
CMDUSAGE("$about [-s|-r]");
_CMDUSAGE("$about [-s|-r]");

static const char *SOURCE = "https://github.com/frolv/lynxbot";
static const char *RELEASE = "https://github.com/frolv/lynxbot/releases/latest";

/* about: print bot information */
std::string CommandHandler::about(char *out, struct command *c)
{
	int opt, type;
	OptionParser op(c->fullCmd, "rs");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "latest-release", NO_ARG, 'r' },
		{ "source", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	type = ABT;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, MAX_MSG, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'r':
			type = REL;
			break;
		case 's':
			type = SRC;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", op.opterr());
			return "";
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length()) {
		_USAGEMSG(out, MAX_MSG, _CMDNAME, _CMDUSAGE);
		return "";
	}

	switch (type) {
	case REL:
	case SRC:
		_sprintf(out, MAX_MSG, "[ABOUT] %s",
				type == REL ? RELEASE : SOURCE);
	default:
		_sprintf(out, MAX_MSG, "[ABOUT] %s is running %s %s. Find out "
				"more at %s", m_name.c_str(), _BOT_NAME,
				BOT_VER, _BOT_WEBSITE);
	}
	return "";
}
