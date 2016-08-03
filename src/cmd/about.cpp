#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define ABT 0
#define REL 1
#define SRC 2

/* full name of the command */
CMDNAME("about");
/* description of the command */
CMDDESCR("print bot information");
/* command usage synopsis */
CMDUSAGE("$about [-s|-r]");

static const char *SOURCE = "https://github.com/frolv/lynxbot";
static const char *RELEASE = "https://github.com/frolv/lynxbot/releases/latest";

/* about: print bot information */
int CommandHandler::about(char *out, struct command *c)
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
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'r':
			type = REL;
			break;
		case 's':
			type = SRC;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (optind != c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	switch (type) {
	case REL:
	case SRC:
		_sprintf(out, MAX_MSG, "[ABOUT] %s",
				type == REL ? RELEASE : SOURCE);
		break;
	default:
		_sprintf(out, MAX_MSG, "[ABOUT] %s is running %s %s. "
				"Find out more at %s", m_name, BOT_NAME,
				BOT_VERSION, BOT_WEBSITE);
		break;
	}
	return EXIT_SUCCESS;
}
