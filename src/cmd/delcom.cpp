#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("delcom");
/* description of the command */
CMDDESCR("delete custom commands");
/* command usage synopsis */
CMDUSAGE("$delcom CMD...");

static void deletecom(CustomCommandHandler *ccmd, const std::string &cmd,
		std::vector<std::string> &del, std::vector<std::string> &inv);
static std::string formatoutput(std::vector<std::string> &del,
		std::vector<std::string> &inv);

/* delcom: delete custom commands */
std::string CommandHandler::delcom(struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string commands;
	std::vector<std::string> argv, del, inv;

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

	if (!m_customCmds->isActive())
		return CMDNAME + ": custom commands are currently disabled";

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	commands = c->fullCmd.substr(op.optind());
	utils::split(commands, ' ', argv);

	for (std::string &cmd : argv)
		deletecom(m_customCmds, cmd, del, inv);
	return "@" + c->nick + ", " + formatoutput(del, inv);
}

/* deletecom: delete a single command */
static void deletecom(CustomCommandHandler *ccmd, const std::string &cmd,
		std::vector<std::string> &del, std::vector<std::string> &inv)
{
	if (ccmd->delcom(cmd))
		del.push_back(cmd);
	else
		inv.push_back(cmd);
}

/* formatoutput: return a formatted output message */
static std::string formatoutput(std::vector<std::string> &del,
		std::vector<std::string> &inv)
{
	std::string output;
	size_t i;

	if (del.size() > 0) {
		output += "deleted: ";
		for (i = 0; i < del.size(); ++i) {
			output += "$" + del[i];
			if (i != del.size() - 1)
				output += " ";
		}
	}
	if (inv.size() > 0) {
		if (!output.empty())
			output += " | ";
		output += "not found: ";
		for (i = 0; i < inv.size(); ++i) {
			output += "$" + inv[i];
			if (i != inv.size() - 1)
				output += " ";
		}
	}
	return output;
}
