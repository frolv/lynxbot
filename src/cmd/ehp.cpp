#include <algorithm>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("ehp");
/* description of the command */
CMDDESCR("view players' ehp");
/* command usage synopsis */
CMDUSAGE("$ehp [-n] [RSN]");

static const std::string CML_HOST = "crystalmathlabs.com";
static const std::string EHP_API = "/tracker/api.php?type="
	"virtualhiscoresatplayer&page=timeplayed&player=";

static const std::string EHP_DESC = "[EHP] EHP stands for efficient hours "
"played. You earn 1 EHP whenever you gain a certain amount of experience in "
"a skill, depending on your level. You can find XP rates here: "
"http://crystalmathlabs.com/tracker/suppliescalc.php . Watch a video "
"explaining EHP: https://www.youtube.com/watch?v=rhxHlO8mvpc";

static bool lookup_player(const std::string &rsn, std::string &res);

/* ehp: view players' ehp */
std::string CommandHandler::ehp(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	std::string rsn, err, res;
	bool usenick;

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

	if (op.optind() == c->fullCmd.length()) {
		if (usenick)
			return CMDNAME + ": no Twitch name given";
		else
			return EHP_DESC;
	}

	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return CMDNAME + ": " + err;
	if (rsn.find(' ') != std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	std::replace(rsn.begin(), rsn.end(), '-', '_');

	if (lookup_player(rsn, res))
		return "[EHP] " + res;
	else
		return CMDNAME + ": " + res;
}

/* lookup_player: look up rsn on cml, return EHP data */
static bool lookup_player(const std::string &rsn, std::string &res)
{
	cpr::Response resp;
	std::vector<std::string> elems;
	size_t dot;

	resp = cpr::Get(cpr::Url("http://" + CML_HOST + EHP_API + rsn),
			cpr::Header{{ "Connection", "close" }});
	if (resp.text == "-3" || resp.text == "-4") {
		res = "could not reach CML API, try again";
		return false;
	}

	utils::split(resp.text, ',', elems);
	if (elems.size() >= 3) {
		std::string &ehp = elems[2];
		if ((dot = ehp.find(".")) != std::string::npos) {
			/* truncate to one decimal place */
			ehp = ehp.substr(0, dot + 2);
		}
		res = "Name: " + elems[1] + ", Rank: " + elems[0] + ", EHP: "
			+ ehp + " (+";
		res += elems.size() == 4 ? elems[3] : "0.00";
		res += " this week)";
		return true;
	} else {
		res = "player '" + rsn + "' does not exist or has not been "
			"tracked on CML";
		return false;
	}
}
