#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../skills.h"
#include "../strfmt.h"

#define NP 0
#define SC 1
#define VH 2
#define MAX_URL 128

/* full name of the command */
_CMDNAME("cml");
/* description of the command */
_CMDDESCR("interact with crystalmathlabs trackers");
/* command usage synopsis */
_CMDUSAGE("$cml [-s|-v] [-nu] [-t SKILL] [RSN]");

static const char *CML_HOST = "https://crystalmathlabs.com";
static const char *CML_USER = "/tracker/track.php?player=";
static const char *CML_SCALC = "/tracker/suppliescalc.php";
static const char *CML_VHS = "/tracker/virtualhiscores.php?page=timeplayed";
static const char *CML_UPDATE = "/tracker/api.php?type=update&player=";
static const char *CML_CTOP = "/tracker/api.php?type=currenttop&count=5&skill=";

static int updatecml(char *rsn, char *err);
static void read_current_top(char *out, int id);

/* cml: interact with crystalmathlabs trackers */
std::string CommandHandler::cml(char *out, struct command *c)
{
	int usenick, update, page, id;
	char buf[RSN_BUF];
	char err[RSN_BUF];

	int opt;
	static struct option long_opts[] = {
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
	page = NP;
	id = -1;
	while ((opt = getopt_long(c->argc, c->argv, "nst:uv", long_opts))
			!= EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'n':
			usenick = 1;
			break;
		case 's':
			page = SC;
			break;
		case 't':
			if (strcmp(optarg, "ehp") == 0) {
				id = 99;
				break;
			}
			if ((id = skill_id(optarg)) == -1) {
				_sprintf(out, MAX_MSG, "%s: invalid skill "
						"name: %s", c->argv[0], optarg);
				return "";
			}
			break;
		case 'u':
			update = 1;
			break;
		case 'v':
			page = VH;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		if (page) {
			if (update || usenick || id != -1)
				_sprintf(out, MAX_MSG, "%s: cannot use other "
						"options with %s", c->argv[0],
						page == SC ? "-s" : "-v");
			else
				_sprintf(out, MAX_MSG, "[CML] %s%s", CML_HOST,
						page == SC ? CML_SCALC : CML_VHS);
		}
		else if (id != -1)
			read_current_top(out, id);
		else if (update)
			_sprintf(out, MAX_MSG, "%s: must provide RSN to update",
					c->argv[0]);
		else if (usenick)
			_sprintf(out, MAX_MSG, "%s: no Twitch nick given",
					c->argv[0]);
		else
			_sprintf(out, MAX_MSG, "[CML] %s", CML_HOST);
		return "";
	} else if (optind != c->argc - 1 || page || id != -1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	/* get the rsn of the queried player */
	if (!getrsn(buf, RSN_BUF, c->argv[optind], c->nick, usenick)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		return "";
	}

	if (update) {
		if (updatecml(buf, err) == 1)
			_sprintf(out, MAX_MSG, "@%s, %s's CML tracker has "
					"been updated", c->nick, buf);
		else
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], err);
	} else {
		_sprintf(out, MAX_MSG, "[CML] %s%s%s", CML_HOST, CML_USER, buf);
	}
	return "";
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
static void read_current_top(char *out, int id)
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
				_CMDNAME);
		return;
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

	if (i != 5)
		_sprintf(orig, MAX_MSG, "%s: could not read current top", _CMDNAME);
}
