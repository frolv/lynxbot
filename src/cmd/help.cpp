#include <algorithm>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("help");
/* description of the command */
CMDDESCR("view command reference manuals");
/* command usage synopsis */
CMDUSAGE("$help CMD");

/* help: view command reference manuals */
std::string CommandHandler::help(struct cmdinfo *c)
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
		return "[HELP] " + path + m_help[cmd] + ".html";

	if (m_defaultCmds.find(cmd) != m_defaultCmds.end())
		return "[HELP] " + path + cmd + ".html";

	if (!(ccmd = m_customCmds->getCom(cmd))->empty())
		return "[HELP] " + cmd + " is a custom command";

	return CMDNAME + ": not a bot command: " + cmd;
}
