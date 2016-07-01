#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("active");
/* description of the command */
CMDDESCR("view current poll");
/* command usage synopsis */
CMDUSAGE("$active");

/* active: view current poll */
std::string CommandHandler::active(struct cmdinfo *c)
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

	return "[ACTIVE] " + (m_activePoll.empty() ? "no poll has been created"
			: m_activePoll);
}
