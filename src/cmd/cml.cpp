#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../skills.h"
#include "../strfmt.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("cml");
/* description of the command */
CMDDESCR("interact with crystalmathlabs trackers");
/* command usage synopsis */
CMDUSAGE("$cml [-s|-v] [-nu] [-t SKILL] [RSN]");

static const char *CML_HOST = "https://crystalmathlabs.com";
static const char *CML_USER = "/tracker/track.php?player=";
static const char *CML_SCALC = "/tracker/suppliescalc.php";
static const char *CML_VHS = "/tracker/virtualhiscores.php?page=timeplayed";
static const char *CML_UPDATE = "/tracker/api.php?type=update&player=";
static const char *CML_CTOP = "/tracker/api.php?type=currenttop&count=5&skill=";

static int updatecml(char *rsn, char *err);
static int read_current_top(char *out, int id);

/* cml: interact with crystalmathlabs trackers */
int CmdHandler::cml(char *out, struct command *c)
{
	int usenick, update, id, status;
	const char *page;
	char buf[RSN_BUF];
	char err[RSN_BUF];

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ "supplies-calc", NO_ARG, 's' },
		{ "current-top", REQ_ARG, 't' },
		{ "update", NO_ARG, 'u' },
		{ "virtual-hiscores", NO_ARG, 'v' },
		{ 0, 0, 0 }
	};

	opt_init();
	usenick = update = 0;
	page = NULL;
	id = -1;
	status = EXIT_SUCCESS;
	while ((opt = l_getopt_long(c->argc, c->argv, "nst:uv", long_opts))
			!= EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'n':
			usenick = 1;
			break;
		case 's':
			page = CML_SCALC;
			break;
		case 't':
			if (strcmp(l_optarg, "ehp") == 0) {
				id = 99;
				break;
			}
			if ((id = skill_id(l_optarg)) == -1) {
				_sprintf(out, MAX_MSG, "%s: invalid skill "
						"name: %s", c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'u':
			update = 1;
			break;
		case 'v':
			page = CML_VHS;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		if (page) {
			if (update || usenick || id != -1) {
				_sprintf(out, MAX_MSG, "%s: cannot use other "
						"options with %s", c->argv[0],
						page == CML_VHS ? "-v" : "-s");
				status = EXIT_FAILURE;
			} else {
				_sprintf(out, MAX_MSG, "[CML] %s%s",
						CML_HOST, page);
			}
		} else if (id != -1) {
			status = read_current_top(out, id);
		} else if (update) {
			_sprintf(out, MAX_MSG, "%s: must provide RSN to update",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else if (usenick) {
			_sprintf(out, MAX_MSG, "%s: no Twitch nick given",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			_sprintf(out, MAX_MSG, "[CML] %s", CML_HOST);
		}
		return status;
	} else if (l_optind != c->argc - 1 || page || id != -1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	/* get the rsn of the queried player */
	if (!getrsn(buf, RSN_BUF, c->argv[l_optind], c->nick, usenick)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		return EXIT_FAILURE;
	}

	if (update) {
		if (updatecml(buf, err) == 1) {
			_sprintf(out, MAX_MSG, "@%s, %s's CML tracker has "
					"been updated", c->nick, buf);
		} else {
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], err);
			status = EXIT_FAILURE;
		}
	} else {
		_sprintf(out, MAX_MSG, "[CML] %s%s%s", CML_HOST, CML_USER, buf);
	}
	return status;
}

/* updatecml: update the cml tracker of player rsn */
static int updatecml(char *rsn, char *err)
{
	int i;
	char url[MAX_URL];
	cpr::Response resp;

	_sprintf(url, MAX_URL, "%s%s%s", CML_HOST, CML_UPDATE, rsn);
	resp = cpr::Get(cpr::Url(url), cpr::Header{{ "Connection", "close" }});

	switch ((i = atoi(resp.text.c_str()))) {
	case 1:
		/* success */
		break;
	case 2:
		_sprintf(err, RSN_BUF, "'%s' could not be found "
				"on the hiscores", rsn);
		break;
	case 3:
		_sprintf(err, RSN_BUF, "'%s' has had a negative "
				"XP gain", rsn);
		break;
	case 4:
		_sprintf(err, RSN_BUF, "unknown error, try again");
		break;
	case 5:
		_sprintf(err, RSN_BUF, "'%s' has been updated within "
				"the last 30s", rsn);
		break;
	case 6:
		_sprintf(err, RSN_BUF, "'%s' is an invalid RSN", rsn);
		break;
	default:
		_sprintf(err, RSN_BUF, "could not reach CML API, try again");
		break;
	}
	return i;
}

/* current_top: get 5 current top players for skill id */
static int read_current_top(char *out, int id)
{
	cpr::Response resp;
	int i;
	char *nick, *score, *s, *orig;
	char url[MAX_URL];
	char text[MAX_URL];

	_sprintf(url, MAX_URL, "%s%s%d", CML_HOST, CML_CTOP, id);
	resp = cpr::Get(cpr::Url(url), cpr::Header{{ "Connection", "close" }});
	strcpy(text, resp.text.c_str());

	if (strcmp(text, "-3") == 0 || strcmp(text, "-4") == 0) {
		_sprintf(out, MAX_MSG, "%s: could not reach CML API, try again",
				CMDNAME);
		return EXIT_FAILURE;
	}

	i = 0;
	nick = text;
	orig = out;
	_sprintf(out, MAX_MSG, "[CML] Current top players in %s: ",
			id == 99 ? "EHP" : skill_name(id).c_str());
	out += strlen(out);
	while ((score = strchr(nick, ','))) {
		++i;
		*score++ = '\0';
		if ((s = strchr(score, '\n')))
			*s = '\0';
		_sprintf(out, MAX_MSG - 50 * i, "%d. %s (+", i, nick);
		out += strlen(out);
		if (id == 99) {
			strcat(out, score);
		} else {
			fmtnum(out, 24, score);
			strcat(out, " xp");
		}
		strcat(out, ")");
		if (i != 5) {
			nick = s + 1;
			strcat(out, ", ");
		}
		out += strlen(out);
	}

	if (i != 5) {
		_sprintf(orig, MAX_MSG, "%s: could not read current top", CMDNAME);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
