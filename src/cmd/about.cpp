#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"
#include "../version.h"

/* full name of the command */
CMDNAME("about");
/* description of the command */
CMDDESCR("print bot information");
/* command usage synopsis */
CMDUSAGE("$about [-s]");

static const std::string SOURCE = "https://github.com/frolv/lynxbot";

/* about: print bot information */
std::string CommandHandler::about(struct cmdinfo *c)
{
	int opt;
	OptionParser op(c->fullCmd, "s");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "source", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 's':
			return "[ABOUT] source: " + SOURCE;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	return "[ABOUT] " + m_name + " is running " + std::string(BOT_NAME)
		+ " " + std::string(BOT_VERSION) + ". Find out more at "
		+ std::string(BOT_WEBSITE);
}
