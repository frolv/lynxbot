#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("8ball");
/* description of the command */
CMDDESCR("respond to questions");
/* command usage synopsis */
CMDUSAGE("$8ball QUESTION");

/* 8ball: respond to questions */
std::string CommandHandler::eightball(struct command *c)
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

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (c->fullCmd[c->fullCmd.length() - 1] != '?')
		return "[8 BALL] Ask me a question.";

	std::uniform_int_distribution<> dis(0, m_eightball.size());
	return "[8 BALL] @" + c->nick + ", " + m_eightball[dis(m_gen)] + ".";
}
