#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("fashiongen");
/* description of the command */
CMDDESCR("generate an outfit");
/* command usage synopsis */
CMDUSAGE("$fashiongen");

/* fashiongen: generate an outfit */
std::string CommandHandler::fashiongen(struct cmdinfo *c)
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

	if (m_fashion.empty())
		return CMDNAME + ": could not read item database";

	return "";
}
