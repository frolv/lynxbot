#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("editcom");
/* description of the command */
CMDDESCR("modify a custom command");
/* command usage synopsis */
CMDUSAGE("$editcom [-c CD] CMD [RESPONSE]");

/* editcom: modify a custom command */
std::string CommandHandler::editcom(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	if (!m_customCmds->isActive())
		return CMDNAME + ": custom commands are currently disabled";

	std::string out, cmd, args;
	time_t cooldown;
	size_t sp;
	bool cd, resp;

	OptionParser op(c->fullCmd, "c:");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	cooldown = -1;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			/* user provided a cooldown */
			try {
				if ((cooldown = std::stoi(op.optarg())) < 0)
					return CMDNAME + ": cooldown cannot be "
						"negative";
			} catch (std::invalid_argument) {
				return CMDNAME  + ": invalid number -- '"
					+ std::string(op.optarg()) + "'";
			}
			break;
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

	out = "@" + c->nick + ", ";
	args = c->fullCmd.substr(op.optind());

	/* determine which parts are being changed */
	cd = cooldown != -1;
	resp = (sp = args.find(' ')) != std::string::npos;
	cmd = resp ? args.substr(0, sp) : args;

	std::string response = resp
		? args.substr(sp + 1) : "";
	/* don't allow reponse to activate a twitch command */
	if (response[0] == '/')
		response = " " + response;
	if (!m_customCmds->editCom(cmd, response, cooldown)) {
		return c->cmd + ": invalid command: $" + cmd;
	} else if (!cd && !resp) {
		out += "command $" + cmd + " was unchanged";
	} else {
		/* build an out string detailing changes */
		out += "command $" + cmd + " has been changed to ";
		if (resp)
			out += "\"" + response + "\""
				+ (cd ? ", with " : ".");
		if (cd) {
			/* reset cooldown in TimerManager */
			m_cooldowns.remove(cmd);
			m_cooldowns.add(cmd, cooldown);
			out += "a " + std::to_string(cooldown)
				+ "s cooldown.";
		}
	}
	return out;
}
