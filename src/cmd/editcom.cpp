#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"
#include "../TimerManager.h"

/* full name of the command */
CMDNAME("editcom");
/* description of the command */
CMDDESCR("modify a custom command");
/* command usage synopsis */
CMDUSAGE("$editcom [-a on|off] [-c CD] [-r] CMD [RESPONSE]");

/* rename flag usage */
static const std::string RUSAGE = "$editcom -r OLD NEW";

static bool edit(CustomCommandHandler *cch, const std::string &args,
		const std::string &set, time_t cooldown, TimerManager *tm,
		std::string &res);
static bool rename(CustomCommandHandler *cch, const std::string &args,
		std::string &res);

/* editcom: modify a custom command */
std::string CommandHandler::editcom(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	if (!m_customCmds->isActive())
		return CMDNAME + ": custom commands are currently disabled";

	std::string out, res, set, cmd;
	time_t cooldown;
	bool ren;

	OptionParser op(c->fullCmd, "a:c:r");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "active", REQ_ARG, 'a'},
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ "rename", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	cooldown = -1;
	ren = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			if ((set = std::string(op.optarg())) != "on"
					&& set != "off")
				return CMDNAME + ": -a setting must be on/off";
			break;
		case 'c':
			/* user provided a cooldown */
			try {
				if ((cooldown = std::stoi(op.optarg())) < 0)
					return CMDNAME + ": cooldown cannot be "
						"negative";
			} catch (std::invalid_argument) {
				return CMDNAME  + ": invalid number -- '"
					+ std::string(op.optarg()) + "'";
			} catch (std::out_of_range) {
				return CMDNAME + ": number too large";
			}
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'r':
			ren = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
			return USAGEMSG(CMDNAME, CMDUSAGE);

	out = "@" + c->nick + ", ";
	if (ren) {
		if (cooldown != -1 || !set.empty())
			return CMDNAME + ": cannot use other flags with -r";
		if (!rename(m_customCmds, c->fullCmd.substr(op.optind()), res)) {
			if (res.empty())
				return USAGEMSG(CMDNAME, RUSAGE);
			else
				return CMDNAME + ": " + res;
		}
		return out + res;
	}

	if (!edit(m_customCmds, c->fullCmd.substr(op.optind()), set,
				cooldown, &m_cooldowns, res))
		return CMDNAME + ": " + res;
	return out + res;
}

/* edit: edit a custom command */
static bool edit(CustomCommandHandler *cch, const std::string &args,
		const std::string &set, time_t cooldown, TimerManager *tm,
		std::string &res)
{
	bool cd, resp, act;
	size_t sp;
	std::string cmd, response;

	/* determine which parts are being changed */
	cd = cooldown != -1;
	resp = (sp = args.find(' ')) != std::string::npos;
	cmd = resp ? args.substr(0, sp) : args;
	act = !set.empty();

	response = resp ? args.substr(sp + 1) : "";

	/* don't allow reponse to activate a twitch command */
	if (response[0] == '/')
		response = " " + response;
	if (!cch->editcom(cmd, response, cooldown)) {
		res = cch->error();
		return false;
	} else if (!cd && !resp && !act) {
		res = "command $" + cmd + " was unchanged";
		return true;
	} else {
		/* build an out string detailing changes */
		res += "command $" + cmd + " has been";
		if (act) {
			if (set == "on") {
				if (!cch->activate(cmd)) {
					res = cch->error();
					res += ". Command not activated.";
					return false;
				}
				res += " activated";
			} else {
				cch->deactivate(cmd);
				res += " deactivated";
			}
		}
		if (resp || cd) {
			res += (act) ? " and" : "";
			res += " changed to ";
			if (resp)
				res += "\"" + response + "\""
					+ (cd ? ", with " : "");
			if (cd) {
				/* reset cooldown in TimerManager */
				tm->remove(cmd);
				tm->add(cmd, cooldown);
				res += "a " + std::to_string(cooldown)
					+ "s cooldown";
			}
		}
		res += ".";
		return true;
	}
}

/* rename: rename a custom command */
static bool rename(CustomCommandHandler *cch, const std::string &args,
		std::string &res)
{
	std::string cmd, newcmd;
	size_t t;

	if ((t = args.find(' ')) == std::string::npos)
		return false;
	cmd = args.substr(0, t);

	if ((newcmd = args.substr(t + 1)).find(' ') != std::string::npos)
		return false;

	if (!cch->rename(cmd, newcmd)) {
		res = cch->error();
		return false;
	}
	res = "command $" + cmd + " has been renamed to $" + newcmd;
	return true;
}
