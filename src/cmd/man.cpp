#include <algorithm>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("man");
/* description of the command */
CMDDESCR("view command reference manuals");
/* command usage synopsis */
CMDUSAGE("$man CMD");

/* man: view command reference manuals */
std::string CommandHandler::man(char *out, struct command *c)
{
	std::string cmd, path;
	Json::Value *ccmd;

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

	if (op.optind() == c->fullCmd.length()
			|| (cmd = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	path = std::string(BOT_WEBSITE) + "/manual/";
	if (m_help.find(cmd) != m_help.end())
		return "[MAN] " + path + m_help[cmd] + ".html";

	if (m_defaultCmds.find(cmd) != m_defaultCmds.end())
		return "[MAN] " + path + cmd + ".html";

	if (!(ccmd = m_customCmds->getcom(cmd))->empty())
		return "[MAN] " + cmd + " is a custom command";

	return CMDNAME + ": no manual entry for '" + cmd + "'";
}
