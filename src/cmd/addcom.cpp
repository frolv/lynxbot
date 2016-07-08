#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("addcom");
/* description of the command */
CMDDESCR("create a new custom command");
/* command usage synopsis */
CMDUSAGE("$addcom [-c CD] CMD RESPONSE");

static bool create(CustomCommandHandler *cch, const std::string &args,
		const std::string &nick, time_t cooldown, std::string &res);

/* addcom: create a new custom command */
std::string CommandHandler::addcom(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	if (!m_customCmds->isActive())
		return CMDNAME + ": custom commands are currently disabled";

	std::string out, res;
	time_t cooldown;

	OptionParser op(c->fullCmd, "c:");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	cooldown = 15;
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
	if (!create(m_customCmds, c->fullCmd.substr(op.optind()),
				c->nick, cooldown, res))
		return CMDNAME + ": " + res;
	return out + res;
}

/* create: create a custom command */
static bool create(CustomCommandHandler *cch, const std::string &args,
		const std::string &nick, time_t cooldown, std::string &res)
{
	size_t sp;
	std::string cmd, response;

	if ((sp = args.find(' ')) == std::string::npos) {
		res = "no response provided for command $" + args;
		return false;
	}

	/* first word is command, rest is response */
	cmd = args.substr(0, sp);
	response = args.substr(sp + 1);

	/* don't allow reponse to activate a twitch command */
	if (response[0] == '/')
		response = " " + response;
	if (!cch->addCom(cmd, response, nick, cooldown)) {
		res = cch->error();
		return false;
	}
	res = "command $" + cmd + " has been added with a "
		+ std::to_string(cooldown) + "s cooldown.";
	return true;
}
