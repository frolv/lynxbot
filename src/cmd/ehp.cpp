#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../strfmt.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("ehp");
/* description of the command */
CMDDESCR("view players' ehp");
/* command usage synopsis */
CMDUSAGE("$ehp [-n] [RSN]");

static const char *CML_HOST = "https://crystalmathlabs.com";
static const char *EHP_API = "/tracker/api.php?type="
	"virtualhiscoresatplayer&page=timeplayed&player=";

static const char *EHP_DESC = "[EHP] EHP stands for efficient hours "
"played. You earn 1 EHP whenever you gain a certain amount of experience in "
"a skill, depending on your level. You can find EHP rates here: "
"http://crystalmathlabs.com/tracker/suppliescalc.php . Watch a video "
"explaining EHP: https://www.youtube.com/watch?v=rhxHlO8mvpc";

static int lookup_player(char *out, const char *rsn);

/* ehp: view players' ehp */
int CmdHandler::ehp(char *out, struct command *c)
{
	int usenick, status;
	char buf[RSN_BUF];

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ 0, 0, 0 }
	};

	usenick = 0;
	status = EXIT_SUCCESS;
	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "n", long_opts)) != EOF) {
		switch (opt) {
		case 'n':
			usenick = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		if (usenick) {
			_sprintf(out, MAX_MSG, "%s: no Twitch name given",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			_sprintf(out, MAX_MSG, "%s", EHP_DESC);
		}
		return status;
	}
	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (!getrsn(buf, RSN_BUF, c->argv[l_optind], c->nick, usenick)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		return EXIT_FAILURE;
	}

	return lookup_player(out, buf);
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
				CMDNAME);
		return EXIT_FAILURE;
	}
	if (strcmp(buf, "-1") == 0) {
		_sprintf(out, MAX_MSG, "%s: player '%s' does not exist or has "
				"not been tracked on CML", CMDNAME, rsn);
		return EXIT_FAILURE;
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

	_sprintf(out, MAX_MSG, "[EHP] Name: %s, Rank: %s, EHP: %s (+%s this "
			"week)", name, rank, num, *week ? week : "0.00");
	return EXIT_SUCCESS;
}
