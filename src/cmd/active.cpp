#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("active");
/* description of the command */
CMDDESCR("view current poll");
/* command usage synopsis */
CMDUSAGE("$active");

static const std::string STRAWPOLL_HOST = "https://strawpoll.me";

/* active: view current poll */
std::string CommandHandler::active(struct command *c)
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

	return "[ACTIVE] " + (m_activePoll.empty() ? "no poll has been created"
			: (STRAWPOLL_HOST + "/" + m_activePoll));
}
