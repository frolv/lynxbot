#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("setrec");
/* description of the command */
CMDDESCR("enable/disabe recurring messages");
/* command usage synopsis */
CMDUSAGE("$setrec on|off");

/* setrec: enable and disable recurring messages */
std::string CommandHandler::setrec(struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string out, set;

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

	out = "@" + c->nick + ", ";
	if (op.optind() == c->fullCmd.length()
			|| ((set = c->fullCmd.substr(op.optind())) != "on"
			&& set != "off"))
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (set == "on") {
		m_evtp->activateMessages();
		out += "recurring messages enabled.";
	} else {
		m_evtp->deactivateMessages();
		out += "recurring messages disabled.";
	}
	return out;
}
