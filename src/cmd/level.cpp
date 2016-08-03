#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
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

static void get_hiscores(char *out, struct command *c, const char *rsn,
		int id, int type);

/* level: look up players' levels */
std::string CommandHandler::level(char *out, struct command *c)
{
	int usenick, id, type;
	char buf[RSN_BUF];

	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "ironman", NO_ARG, 'i' },
		{ "nick", NO_ARG, 'n' },
		{ "ultimate", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	opt_init();
	usenick = 0;
	type = REG;
	while ((opt = getopt_long(c->argc, c->argv, "inu", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
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
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc - 2)
		USAGEMSG(out, CMDNAME, CMDUSAGE);
	else if (!getrsn(buf, RSN_BUF, c->argv[optind + 1], c->nick, usenick))
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
	else if ((id = skill_id(c->argv[optind])) == -1)
		_sprintf(out, MAX_MSG, "%s: invalid skill name: %s",
				c->argv[0], c->argv[optind]);
	else
		get_hiscores(out, c, buf, id, type);

	return "";
	/* skill nickname is displayed to save space */
	/* nick = skill_nick(id); */
	/* std::transform(nick.begin(), nick.end(), nick.begin(), toupper); */

	/* return "[" + nick + "] " + which + "Name: " + rsn + ", " + res; */
}

/* get_hiscores: return hiscore data of player rsn in skill id */
static void get_hiscores(char *out, struct command *c, const char *rsn,
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
		_sprintf(out, MAX_MSG, "%s: player '%s' not found on "
				"%shiscores", c->argv[0], rsn,
				type == REG ? "" : type == IRON ? "ironman "
				: "ultimate ironman ");
		return;
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
	_sprintf(nick, 8, "%s", skill_nick(id).c_str());
	for (i = 0; nick[i]; ++i)
		nick[i] = toupper(nick[i]);

	_sprintf(out, MAX_MSG, "[%s]%s Name: %s, Level: %s, Exp: %s, Rank: %s",
			nick, type == REG ? "" : type == IRON ? " (iron)"
			: " (ult)", rsn, t, exp, rnk);
}
