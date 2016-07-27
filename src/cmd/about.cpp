#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define ABT 0
#define REL 1
#define SRC 2

/* full name of the command */
CMDNAME("about");
/* description of the command */
CMDDESCR("print bot information");
/* command usage synopsis */
CMDUSAGE("$about [-s|-r]");

static const std::string SOURCE = "https://github.com/frolv/lynxbot";
static const std::string RELEASE =
	"https://github.com/frolv/lynxbot/releases/latest";

/* about: print bot information */
std::string CommandHandler::about(struct command *c)
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
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'r':
			type = REL;
			break;
		case 's':
			type = SRC;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	switch (type) {
	case REL:
		return "[ABOUT] " + RELEASE;
	case SRC:
		return "[ABOUT] " + SOURCE;
	default:
		return "[ABOUT] " + m_name + " is running "
			+ std::string(BOT_NAME) + " " + std::string(BOT_VERSION)
			+ ". Find out more at " + std::string(BOT_WEBSITE);
	}
}
