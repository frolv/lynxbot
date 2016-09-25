#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../skills.h"
#include "../strfmt.h"
#include "../stringparse.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("cml");
/* description of the command */
CMDDESCR("interact with crystalmathlabs trackers");
/* command usage synopsis */
CMDUSAGE("$cml [-s|-v] [-nu] [RSN]");
/* -t flag usage */
static const char *TUSAGE = "$cml -t [-c AMT] [-p PERIOD] SKILL";

static const char *CML_HOST = "https://crystalmathlabs.com";
static const char *CML_USER = "/tracker/track.php?player=";
static const char *CML_SCALC = "/tracker/suppliescalc.php";
static const char *CML_VHS = "/tracker/virtualhiscores.php?page=timeplayed";
static const char *CML_UPDATE = "/tracker/api.php?type=update&player=";
static const char *CML_CTOP = "/tracker/api.php?type=currenttop";

static int updatecml(char *rsn, char *err);
static int read_current_top(char *out, int id, int count, const char *period);
static int parse_period(char *s, const char **period);

/* cml: interact with crystalmathlabs trackers */
int CmdHandler::cml(char *out, struct command *c)
{
	int usenick, update, id, status, set;
	int64_t count;
	const char *page, *period;
	char buf[RSN_BUF];
	char err[RSN_BUF];

	int opt;
	static struct l_option long_opts[] = {
		{ "count", REQ_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ "period", REQ_ARG, 'p' },
		{ "supplies-calc", NO_ARG, 's' },
		{ "current-top", NO_ARG, 't' },
		{ "update", NO_ARG, 'u' },
		{ "virtual-hiscores", NO_ARG, 'v' },
		{ 0, 0, 0 }
	};

	opt_init();
	usenick = update = id = set = 0;
	page = NULL;
	status = EXIT_SUCCESS;

	/* display 5 current top players for week by default */
	count = 5;
	period = "week";

	while ((opt = l_getopt_long(c->argc, c->argv, "c:np:stuv", long_opts))
			!= EOF) {
		switch (opt) {
		case 'c':
			if (!parsenum(l_optarg, &count)) {
				snprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (count < 1 || count > 7) {
				snprintf(out, MAX_MSG, "%s: count must be "
						"between 1 and 7", c->argv[0]);
				return EXIT_FAILURE;
			}
			set = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'n':
			usenick = 1;
			break;
		case 'p':
			if (!parse_period(l_optarg, &period)) {
				snprintf(out, MAX_MSG, "%s: invalid period: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			set = 1;
			break;
		case 's':
			page = CML_SCALC;
			break;
		case 't':
			id = 1;
			break;
		case 'u':
			update = 1;
			break;
		case 'v':
			page = CML_VHS;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (set && !id) {
		snprintf(out, MAX_MSG, "%s: -c and -p don't make sense "
				"without -t", c->argv[0]);
		return EXIT_FAILURE;
	}

	if (l_optind == c->argc) {
		if (page) {
			if (update || usenick || id) {
				snprintf(out, MAX_MSG, "%s: cannot use other "
						"options with %s", c->argv[0],
						page == CML_VHS ? "-v" : "-s");
				status = EXIT_FAILURE;
			} else {
				snprintf(out, MAX_MSG, "[CML] %s%s",
						CML_HOST, page);
			}
		} else if (id) {
			USAGEMSG(out, CMDNAME, TUSAGE);
			status = EXIT_FAILURE;
		} else if (update) {
			snprintf(out, MAX_MSG, "%s: must provide RSN to update",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else if (usenick) {
			snprintf(out, MAX_MSG, "%s: no Twitch nick given",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			snprintf(out, MAX_MSG, "[CML] %s", CML_HOST);
		}
		return status;
	} else if (l_optind != c->argc - 1 || page) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (id) {
		if (usenick || update || page) {
			USAGEMSG(out, CMDNAME, TUSAGE);
			return EXIT_FAILURE;
		}
		if (strcmp(c->argv[l_optind], "ehp") == 0) {
			id = 99;
		} else if ((id = skill_id(c->argv[l_optind])) == -1) {
			snprintf(out, MAX_MSG, "%s: invalid skill "
					"name: %s", c->argv[0],
					c->argv[l_optind]);
			return EXIT_FAILURE;
		}
		return read_current_top(out, id, count, period);
	}

	/* get the rsn of the queried player */
	if (!getrsn(buf, RSN_BUF, c->argv[l_optind], c->nick, usenick)) {
		snprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		return EXIT_FAILURE;
	}

	if (update) {
		if (updatecml(buf, err) == 1) {
			snprintf(out, MAX_MSG, "@%s, %s's CML tracker has "
					"been updated.", c->nick, buf);
		} else {
			snprintf(out, MAX_MSG, "%s: %s", c->argv[0], err);
			status = EXIT_FAILURE;
		}
	} else {
		snprintf(out, MAX_MSG, "[CML] %s%s%s", CML_HOST, CML_USER, buf);
	}
	return status;
}

/* updatecml: update the cml tracker of player rsn */
static int updatecml(char *rsn, char *err)
{
	int i;
	char url[MAX_URL];
	cpr::Response resp;

	snprintf(url, MAX_URL, "%s%s%s", CML_HOST, CML_UPDATE, rsn);
	resp = cpr::Get(cpr::Url(url), cpr::Header{{ "Connection", "close" }});

	switch ((i = atoi(resp.text.c_str()))) {
	case 1:
		/* success */
		break;
	case 2:
		snprintf(err, RSN_BUF, "'%s' could not be found "
				"on the hiscores", rsn);
		break;
	case 3:
		snprintf(err, RSN_BUF, "'%s' has had a negative "
				"XP gain", rsn);
		break;
	case 4:
		snprintf(err, RSN_BUF, "unknown error, try again");
		break;
	case 5:
		snprintf(err, RSN_BUF, "'%s' has been updated within "
				"the last 30s", rsn);
		break;
	case 6:
		snprintf(err, RSN_BUF, "'%s' is an invalid RSN", rsn);
		break;
	default:
		snprintf(err, RSN_BUF, "could not reach CML API, try again");
		break;
	}
	return i;
}

/* current_top: get count current top players for skill id */
static int read_current_top(char *out, int id, int count, const char *period)
{
	cpr::Response resp;
	int i;
	char *nick, *score, *s, *orig;
	char url[MAX_URL];
	char text[MAX_URL];

	snprintf(url, MAX_URL, "%s%s&count=%d&skill=%d&timeperiod=%s",
			CML_HOST, CML_CTOP, count, id, period);
	resp = cpr::Get(cpr::Url(url), cpr::Header{{ "Connection", "close" }});
	strcpy(text, resp.text.c_str());

	if (strcmp(text, "-3") == 0 || strcmp(text, "-4") == 0) {
		snprintf(out, MAX_MSG, "%s: could not reach CML API, try again",
				CMDNAME);
		return EXIT_FAILURE;
	}

	nick = text;
	orig = out;
	snprintf(out, MAX_MSG, "[CML] Current top players in %s (%s): ",
			id == 99 ? "EHP" : skill_name(id).c_str(), period);
	out += strlen(out);
	for (i = 0; i < count && (score = strchr(nick, ',')); ++i) {
		*score++ = '\0';
		if ((s = strchr(score, '\n')))
			*s = '\0';
		snprintf(out, MAX_MSG - 50 * i, "%d. %s (+", i + 1, nick);
		out += strlen(out);
		if (id == 99) {
			strcat(out, score);
		} else {
			fmtnum(out, 24, score);
			strcat(out, " xp");
		}
		strcat(out, ")");
		if (i != count - 1) {
			nick = s + 1;
			strcat(out, ", ");
		}
		out += strlen(out);
	}

	if (i != count) {
		snprintf(orig, MAX_MSG, "%s: could not read current top", CMDNAME);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int parse_period(char *s, const char **period)
{
	if (strcmp(s, "day") == 0 || strcmp(s, "d") == 0) {
		*period = "day";
		return 1;
	}
	if (strcmp(s, "week") == 0 || strcmp(s, "w") == 0) {
		*period = "week";
		return 1;
	}
	if (strcmp(s, "month") == 0 || strcmp(s, "m") == 0) {
		*period = "month";
		return 1;
	}

	*period = NULL;
	return 0;
}
