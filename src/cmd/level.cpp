#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"
#include "../skills.h"

/* full name of the command */
CMDNAME("level");
/* description of the command */
CMDDESCR("look up players' levels");
/* command usage synopsis */
CMDUSAGE("$level [-n] SKILL RSN");

static const std::string HS_API =
"http://services.runescape.com/m=hiscore_oldschool/index_lite.ws?player=";

static std::string get_hiscores(const std::string &rsn, int8_t id,
		std::string &err);

/* level: look up players' levels */
std::string CommandHandler::level(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	std::string rsn, err, nick, res;
	std::vector<std::string> argv;
	bool usenick;
	int8_t id;

	OptionParser op(c->fullCmd, "n");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ 0, 0, 0 }
	};

	usenick = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'n':
			usenick = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	utils::split(c->fullCmd.substr(op.optind()), ' ', argv);
	if (argv.size() != 2)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if ((rsn = getRSN(argv[1], c->nick, err, usenick)).empty())
		return err;
	std::replace(rsn.begin(), rsn.end(), '-', '_');

	if ((id = skill_id(argv[0])) == -1)
		return c->cmd + ": invalid skill name: " + argv[0];

	if ((res = get_hiscores(rsn, id, err)).empty())
		return c->cmd + ": " + err;

	/* skill nickname is displayed to save space */
	nick = skill_nick(id);
	std::transform(nick.begin(), nick.end(), nick.begin(), toupper);

	return "[" + nick + "] Name: " + rsn + ", " + res;
}

/* get_hiscores: return hiscore data of player rsn in skill */
static std::string get_hiscores(const std::string &rsn, int8_t id,
		std::string &err)
{
	cpr::Response resp;
	std::vector<std::string> skills;
	std::vector<std::string> tokens;

	resp = cpr::Get(cpr::Url(HS_API + rsn),
			cpr::Header{{ "Connection", "close" }});
	if (resp.text.find("404 - Page not found") != std::string::npos) {
		err = "player '" + rsn + "' not found on hiscores";
		return "";
	}

	utils::split(resp.text, '\n', skills);
	utils::split(skills[id], ',', tokens);

	return "Level: " + tokens[1] + ", Exp: "
		+ utils::formatInteger(tokens[2]) + ", Rank: "
		+ utils::formatInteger(tokens[0]);
}
