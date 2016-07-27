#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("addrec");
/* description of the command */
CMDDESCR("add a recurring message");
/* command usage synopsis */
CMDUSAGE("$addrec [-c INT] MSG");

/* addrec: add a recurring message */
std::string CommandHandler::addrec(struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string output = "@" + c->nick + ", ";
	time_t cooldown = 300;
	int opt;
	OptionParser op(c->fullCmd, "c:");
	static struct OptionParser::option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "interval", REQ_ARG, 'c' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			try {
				cooldown = 60 * std::stoi(op.optarg());
			} catch (std::invalid_argument) {
				return CMDNAME + ": invalid number -- '"
					+ op.optarg() + "'";
			} catch (std::out_of_range) {
				return CMDNAME + ": number too large";
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

	if (cooldown % 300 != 0)
		return CMDNAME + ": interval must be a multiple of 5 mins";
	else if (cooldown > 3600)
		return CMDNAME + ": interval cannot be longer than 60 mins";

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);
	else if (!m_evtp->addMessage(c->fullCmd.substr(op.optind()), cooldown))
		return CMDNAME + ": limit of 5 recurring messages reached";
	else
		output += "recurring message \""
			+ c->fullCmd.substr(op.optind())
			+ "\" has been added at a "
			+ std::to_string(cooldown / 60) + " min interval.";
	return output;
}
