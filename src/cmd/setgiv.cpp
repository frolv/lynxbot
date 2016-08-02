#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../Giveaway.h"
#include "../option.h"
#include "../stringparse.h"

#define ALL	(-1)
#define FOLLOW	1
#define TIMED	2
#define IMAGE	3

/* full name of the command */
_CMDNAME("setgiv");
/* description of the command */
_CMDDESCR("change giveaway settings");
/* command usage synopsis */
_CMDUSAGE("$setgiv [-f|-i|-t] [-n LIM] on|off|check");

static void process(char *out, Giveaway *g, struct command *c,
		int type, int amt);

/* setgiv: change giveaway settings */
std::string CommandHandler::setgiv(char *out, struct command *c)
{
	int setfollowers, settimer, setimages, type;
	int64_t amt;

	int opt;
	static struct option long_opts[] = {
		{ "followers", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "image", NO_ARG, 'i' },
		{ "amount", REQ_ARG, 'n' },
		{ "timed", NO_ARG, 't' },
		{ 0, 0, 0 }
	};

	opt_init();
	amt = setfollowers = settimer = setimages = 0;
	while ((opt = getopt_long(c->argc, c->argv, "fin:t", long_opts))
			!= EOF) {
		switch (opt) {
		case 'f':
			setfollowers = 1;
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'i':
			setimages = 1;
			break;
		case 'n':
			if (!parsenum(optarg, &amt)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], optarg);
				return "";
			}
			if (amt < 1) {
				_sprintf(out, MAX_MSG, "%s: amount must be a "
						"postive integer", c->argv[0]);
				return "";
			}
			break;
		case 't':
			settimer = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if ((setfollowers && settimer) || (setfollowers && setimages)
			|| (setimages && settimer)) {
		_sprintf(out, MAX_MSG, "%s: cannot combine -f, -i and -t flags",
				c->argv[0]);
		return "";
	}

	if (optind != c->argc - 1 || (strcmp(c->argv[optind], "on") != 0
				&& strcmp(c->argv[optind], "off") != 0
				&& strcmp(c->argv[optind], "check") != 0)) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

#ifdef _WIN32
	if (setimages) {
		_sprintf(out, MAX_MSG, "%s: image-based giveaways are not "
				"available on Windows systems", c->argv[0]);
		return "";
	}
#endif

	type = ALL;
	if (setfollowers)
		type = FOLLOW;
	if (settimer)
		type = TIMED;
	if (setimages)
		type = IMAGE;

	if (strcmp(c->argv[optind], "check") == 0) {
		_sprintf(out, MAX_MSG, "@%s, %s", c->nick,
				m_givp->currentSettings(type).c_str());
		return "";
	}

	/* allow all users to check but only owner to set */
	if (!P_ISOWN(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	process(out, m_givp, c, type, amt);
	return "";
}

/* process: perform the giveaway setting */
static void process(char *out, Giveaway *g, struct command *c,
		int type, int amt)
{
	char *s;

	_sprintf(out, MAX_MSG, "@%s, ", c->nick);
	s = strchr(out, '\0');

	if (strcmp(c->argv[optind], "on") == 0) {
		switch (type) {
		case FOLLOW:
			g->setFollowers(true, amt);
			_sprintf(s, MAX_MSG, "giveaways set to occur every "
					"%d followers.", g->followers());
			break;
		case TIMED:
			g->setTimer(true, (time_t)amt * 60);
			_sprintf(s, MAX_MSG, "giveaways set to occur every "
					"%d minutes.", amt);
			break;
		case IMAGE:
			g->setImages(true);
			_sprintf(s, MAX_MSG, "image-based giveaways enabled.");
			break;
		default:
			if (!g->activate(time(nullptr)))
				_sprintf(out, MAX_MSG, "%s: %s",
						c->argv[0], g->err());
			else
				_sprintf(s, MAX_MSG, "giveaways have "
						"been activated");
			break;
		}
	} else {
		switch (type) {
		case FOLLOW:
			g->setFollowers(false);
			_sprintf(s, MAX_MSG, "follower giveaways disabled.");
			break;
		case TIMED:
			g->setTimer(false);
			_sprintf(s, MAX_MSG, "timed giveaways disabled.");
			break;
		case IMAGE:
			g->setImages(false);
			_sprintf(s, MAX_MSG, "image-based giveaways disabled.");
			break;
		default:
			g->deactivate();
			_sprintf(s, MAX_MSG, "giveaways deactivated.");
			break;
		}
	}
}
