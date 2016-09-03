#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../Giveaway.h"
#include "../option.h"
#include "../stringparse.h"

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

static int process(char *out, Giveaway *g, struct command *c,
		int type, int amt);

/* setgiv: change giveaway settings */
int CmdHandler::setgiv(char *out, struct command *c)
{
	int setfollowers, settimer, setimages, type;
	int64_t amt;

	int opt;
	static struct l_option long_opts[] = {
		{ "followers", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "image", NO_ARG, 'i' },
		{ "amount", REQ_ARG, 'n' },
		{ "timed", NO_ARG, 't' },
		{ 0, 0, 0 }
	};

	opt_init();
	amt = setfollowers = settimer = setimages = 0;
	while ((opt = l_getopt_long(c->argc, c->argv, "fin:t", long_opts))
			!= EOF) {
		switch (opt) {
		case 'f':
			setfollowers = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'i':
			setimages = 1;
			break;
		case 'n':
			if (!parsenum(l_optarg, &amt)) {
				snprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (amt < 1) {
				snprintf(out, MAX_MSG, "%s: amount must be a "
						"postive integer", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 't':
			settimer = 1;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if ((setfollowers && settimer) || (setfollowers && setimages)
			|| (setimages && settimer)) {
		snprintf(out, MAX_MSG, "%s: cannot combine -f, -i and -t flags",
				c->argv[0]);
		return EXIT_FAILURE;
	}

	if (l_optind != c->argc - 1 || (strcmp(c->argv[l_optind], "on") != 0
				&& strcmp(c->argv[l_optind], "off") != 0
				&& strcmp(c->argv[l_optind], "check") != 0)) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	if (setimages) {
		snprintf(out, MAX_MSG, "%s: image-based giveaways are not "
				"available on Windows systems", c->argv[0]);
		return EXIT_FAILURE;
	}
#endif

	type = ALL;
	if (setfollowers)
		type = FOLLOW;
	if (settimer)
		type = TIMED;
	if (setimages)
		type = IMAGE;

	if (strcmp(c->argv[l_optind], "check") == 0) {
		snprintf(out, MAX_MSG, "@%s, %s", c->nick,
				m_givp->currentSettings(type).c_str());
		return EXIT_SUCCESS;
	}

	/* allow all users to check but only owner to set */
	if (!P_ISOWN(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	return process(out, m_givp, c, type, amt);
}

/* process: perform the giveaway setting */
static int process(char *out, Giveaway *g, struct command *c,
		int type, int amt)
{
	char *s;

	snprintf(out, MAX_MSG, "@%s, ", c->nick);
	s = strchr(out, '\0');

	if (strcmp(c->argv[l_optind], "on") == 0) {
		switch (type) {
		case FOLLOW:
			g->setFollowers(true, amt);
			snprintf(s, MAX_MSG, "giveaways set to occur every "
					"%d followers.", g->followers());
			break;
		case TIMED:
			g->setTimer(true, (time_t)amt * 60);
			snprintf(s, MAX_MSG, "giveaways set to occur every "
					"%d minutes.", amt);
			break;
		case IMAGE:
			g->setImages(true);
			snprintf(s, MAX_MSG, "image-based giveaways enabled.");
			break;
		default:
			if (!g->activate(time(nullptr))) {
				snprintf(out, MAX_MSG, "%s: %s",
						c->argv[0], g->err());
				return EXIT_FAILURE;
			}
			snprintf(s, MAX_MSG, "giveaways have been activated");
			break;
		}
	} else {
		switch (type) {
		case FOLLOW:
			g->setFollowers(false);
			snprintf(s, MAX_MSG, "follower giveaways disabled.");
			break;
		case TIMED:
			g->setTimer(false);
			snprintf(s, MAX_MSG, "timed giveaways disabled.");
			break;
		case IMAGE:
			g->setImages(false);
			snprintf(s, MAX_MSG, "image-based giveaways disabled.");
			break;
		default:
			g->deactivate();
			snprintf(s, MAX_MSG, "giveaways deactivated.");
			break;
		}
	}
	return EXIT_SUCCESS;
}
