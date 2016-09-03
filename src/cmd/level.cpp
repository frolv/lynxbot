#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../skills.h"
#include "../strfmt.h"

#define REG  0
#define IRON 1
#define ULT  2

/* full name of the command */
CMDNAME("level");
/* description of the command */
CMDDESCR("look up players' levels");
/* command usage synopsis */
CMDUSAGE("$level [-i|-u] [-n] SKILL RSN");

static const char *HS_BASE =
	"http://services.runescape.com/m=hiscore_oldschool";
static const char *HS_IRON = "_ironman";
static const char *HS_ULT = "_ultimate";
static const char *HS_QUERY = "/index_lite.ws?player=";

static int get_hiscores(char *out, struct command *c, const char *rsn,
		int id, int type);

/* level: look up players' levels */
int CmdHandler::level(char *out, struct command *c)
{
	int usenick, id, type, status;
	char buf[RSN_BUF];

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "ironman", NO_ARG, 'i' },
		{ "nick", NO_ARG, 'n' },
		{ "ultimate", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	opt_init();
	usenick = 0;
	type = REG;
	status = EXIT_SUCCESS;
	while ((opt = l_getopt_long(c->argc, c->argv, "inu", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_FAILURE;
		case 'i':
			type = IRON;
			break;
		case 'n':
			usenick = 1;
			break;
		case 'u':
			type = ULT;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc - 2) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		status = EXIT_FAILURE;
	} else if (!getrsn(buf, RSN_BUF, c->argv[l_optind + 1],
				c->nick, usenick)) {
		snprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
		status = EXIT_FAILURE;
	} else if ((id = skill_id(c->argv[l_optind])) == -1) {
		snprintf(out, MAX_MSG, "%s: invalid skill name: %s",
				c->argv[0], c->argv[l_optind]);
		status = EXIT_FAILURE;
	} else {
		status = get_hiscores(out, c, buf, id, type);
	}

	return status;
}

/* get_hiscores: return hiscore data of player rsn in skill id */
static int get_hiscores(char *out, struct command *c, const char *rsn,
		int id, int type)
{
	cpr::Response resp;
	char buf[MAX_MSG];
	char exp[RSN_BUF];
	char rnk[RSN_BUF];
	char nick[8];
	char *s, *t, *u;
	int i;

	strcpy(buf, HS_BASE);
	if (type == IRON)
		strcat(buf, HS_IRON);
	else if (type == ULT)
		strcat(buf, HS_ULT);
	strcat(buf, HS_QUERY);
	strcat(buf, rsn);

	resp = cpr::Get(cpr::Url(buf), cpr::Header{{ "Connection", "close" }});
	strcpy(buf, resp.text.c_str());

	if (strstr(buf, "404 - Page not found")) {
		snprintf(out, MAX_MSG, "%s: player '%s' not found on "
				"%shiscores", c->argv[0], rsn,
				type == REG ? "" : type == IRON ? "ironman "
				: "ultimate ironman ");
		return EXIT_FAILURE;
	}

	s = buf;
	for (i = 0; i < id; ++i)
		s = strchr(s, '\n') + 1;
	if ((t = strchr(s, '\n')))
		*t = '\0';

	t = strchr(s, ',');
		*t++ = '\0';
	u = strchr(t, ',');
		*u++ = '\0';
	fmtnum(rnk, RSN_BUF, s);
	fmtnum(exp, RSN_BUF, u);
	snprintf(nick, 8, "%s", skill_nick(id).c_str());
	for (i = 0; nick[i]; ++i)
		nick[i] = toupper(nick[i]);

	snprintf(out, MAX_MSG, "[%s]%s Name: %s, Level: %s, Exp: %s, Rank: %s",
			nick, type == REG ? "" : type == IRON ? " (iron)"
			: " (ult)", rsn, t, exp, rnk);
	return EXIT_SUCCESS;
}
