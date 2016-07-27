#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("manual");
/* description of the command */
CMDDESCR("view the lynxbot manual");
/* command usage synopsis */
CMDUSAGE("$manual");

/* manual: view the lynxbot manual */
std::string CommandHandler::manual(struct command *c)
{
	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	return "[MANUAL] " + std::string(BOT_WEBSITE) + "/manual/index.html";
}
