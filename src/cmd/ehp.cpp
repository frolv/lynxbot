#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../strfmt.h"

#define MAX_URL 128

/* full name of the command */
_CMDNAME("ehp");
/* description of the command */
_CMDDESCR("view players' ehp");
/* command usage synopsis */
_CMDUSAGE("$ehp [-n] [RSN]");

static const char *CML_HOST = "https://crystalmathlabs.com";
static const char *EHP_API = "/tracker/api.php?type="
	"virtualhiscoresatplayer&page=timeplayed&player=";

static const char *EHP_DESC = "[EHP] EHP stands for efficient hours "
"played. You earn 1 EHP whenever you gain a certain amount of experience in "
"a skill, depending on your level. You can find XP rates here: "
"http://crystalmathlabs.com/tracker/suppliescalc.php . Watch a video "
"explaining EHP: https://www.youtube.com/watch?v=rhxHlO8mvpc";

static int lookup_player(char *out, const char *rsn);

/* ehp: view players' ehp */
std::string CommandHandler::ehp(char *out, struct command *c)
{
	int usenick;
	char buf[RSN_BUF];

	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ 0, 0, 0 }
	};

	usenick = 0;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "n", long_opts)) != EOF) {
		switch (opt) {
		case 'n':
			usenick = 1;
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		if (usenick)
			_sprintf(out, MAX_MSG, "%s: no Twitch name given",
					c->argv[0]);
		else
			_sprintf(out, MAX_MSG, "%s", EHP_DESC);
		return "";
	}
	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (!getrsn(buf, RSN_BUF, c->argv[optind], c->nick, usenick)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		return "";
	}

	lookup_player(out, buf);
	return "";
}

/* lookup_player: look up rsn on cml, write EHP data to out */
static int lookup_player(char *out, const char *rsn)
{
	cpr::Response resp;
	char buf[MAX_URL];
	char num[MAX_URL];
	char *name, *rank, *ehp, *week, *s;

	_sprintf(buf, MAX_URL, "%s%s%s", CML_HOST, EHP_API, rsn);
	resp = cpr::Get(cpr::Url(buf), cpr::Header{{ "Connection", "close" }});
	strcpy(buf, resp.text.c_str());

	if (strcmp(buf, "-3") == 0 || strcmp(buf, "-4") == 0) {
		_sprintf(out, MAX_MSG, "%s: could not reach CML API, try again",
				_CMDNAME);
		return 0;
	}
	if (strcmp(buf, "-1") == 0) {
		_sprintf(out, MAX_MSG, "%s: player '%s' does not exist or has "
				"not been tracked on CML", _CMDNAME, rsn);
		return 0;
	}

	/* extract and format data from buf */
	rank = buf;
	name = strchr(rank, ',');
	*name++ = '\0';
	ehp = strchr(name, ',');
	*ehp++ = '\0';
	week = strchr(ehp, ',');
	*week++ = '\0';
	if ((s = strchr(ehp, '.')))
		s[3] = '\0';
	fmtnum(num, MAX_URL, ehp);

	_sprintf(out, MAX_MSG, "Name: %s, Rank: %s, EHP: %s (+%s this week)",
			name, rank, num, *week ? week : "0.00");

	return 1;
}
