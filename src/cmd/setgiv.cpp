#include "command.h"
#include "../CommandHandler.h"
#include "../Giveaway.h"
#include "../OptionParser.h"

#define ALL	(-1)
#define FOLLOW	1
#define TIMED	2
#define IMAGE	3

/* full name of the command */
CMDNAME("setgiv");
/* description of the command */
CMDDESCR("change giveaway settings");
/* command usage synopsis */
CMDUSAGE("$setgiv [-f|-i|-t] [-n LIM] on|off|check");

static std::string process(Giveaway *g, int8_t type, const std::string &set,
		const std::string &nick, int amt);

/* setgiv: change giveaway settings */
std::string CommandHandler::setgiv(char *out, struct command *c)
{
	std::string set;
	bool setfollowers, settimer, setimages;
	int8_t type;

	OptionParser op(c->fullCmd, "fin:t");
	int opt, amt;
	static struct OptionParser::option long_opts[] = {
		{ "followers", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "image", NO_ARG, 'i' },
		{ "amount", REQ_ARG, 'n' },
		{ "timed", NO_ARG, 't' },
		{ 0, 0, 0 }
	};

	amt = 0;
	setfollowers = settimer = setimages = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'f':
			setfollowers = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'i':
			setimages = true;
			break;
		case 'n':
			try {
				if ((amt = std::stoi(op.optarg())) <= 0)
					return c->cmd + ": amount must be a "
						"positive integer";
			} catch (std::invalid_argument) {
				return c->cmd + ": invalid number -- '"
					+ op.optarg() + "'";
			}
			break;
		case 't':
			settimer = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if ((setfollowers && settimer) || (setfollowers && setimages)
			|| (setimages && settimer))
		return CMDNAME + ": cannot combine -f, -i and -t flags";

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	set = c->fullCmd.substr(op.optind());
	if (set != "on" && set != "off" && set != "check")
		return USAGEMSG(CMDNAME, CMDUSAGE);

#ifdef _WIN32
	if (setimages)
		return CMDNAME + ": image-based giveaways are not available "
			"on Windows systems";
#endif

	type = ALL;
	if (setfollowers)
		type = FOLLOW;
	if (settimer)
		type = TIMED;
	if (setimages)
		type = IMAGE;

	if (set == "check")
		return "@" + std::string(c->nick) + ", " + m_givp->currentSettings(type);

	/* allow all users to check but only owner to set */
	if (!P_ISOWN(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	return process(m_givp, type, set, c->nick, amt);
}

/* process: perform the giveaway setting */
static std::string process(Giveaway *g, int8_t type, const std::string &set,
		const std::string &nick, int amt)
{
	std::string res, err;

	res = "@" + nick + ", ";
	if (set == "on") {
		switch (type) {
		case FOLLOW:
			g->setFollowers(true, amt);
			res += "giveaways set to occur every ";
			res += std::to_string(g->followers());
			res += " followers.";
			break;
		case TIMED:
			g->setTimer(true, (time_t)amt * 60);
			res += "giveaways set to occur every ";
			res += std::to_string(g->interval() / 60);
			res += " minutes.";
			break;
		case IMAGE:
			g->setImages(true);
			res += "image-based giveaways enabled.";
			break;
		default:
			g->activate(time(nullptr), err);
			res += "giveaways have been activated.";
			break;
		}
	} else {
		switch (type) {
		case FOLLOW:
			g->setFollowers(false);
			res += "follower giveaways have been disabled.";
			break;
		case TIMED:
			g->setTimer(false);
			res += "timed giveaways have been disabled.";
			break;
		case IMAGE:
			g->setImages(false);
			res += "image-based giveaways disabled.";
			break;
		default:
			g->deactivate();
			res += "giveaways have been deactivated.";
			break;
		}
	}
	if (!err.empty())
		return CMDNAME + ": " + err;

	return res;
}
