#include <cpr/cpr.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define NP 0
#define SC 1
#define VH 2

/* full name of the command */
CMDNAME("cml");
/* description of the command */
CMDDESCR("interact with crystalmathlabs trackers");
/* command usage synopsis */
CMDUSAGE("$cml [-s|-v] [-nu] [RSN]");

static const std::string CML_HOST = "https://crystalmathlabs.com";
static const std::string CML_USER = "/tracker/track.php?player=";
static const std::string CML_SCALC = "/tracker/suppliescalc.php";
static const std::string CML_VHS = "/tracker/virtualhiscores.php?page=timeplayed";
static const std::string CML_UPDATE = "/tracker/api.php?type=update&player=";

static int updatecml(const std::string &rsn, std::string &err);

/* cml: interact with crystalmathlabs trackers */
std::string CommandHandler::cml(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	std::string rsn, err;
	bool usenick, update;
	int page;

	OptionParser op(c->fullCmd, "nsuv");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ "virtual-hiscores", NO_ARG, 'v' },
		{ "suppliescalc", NO_ARG, 's' },
		{ "update", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	usenick = update = false;
	page = NP;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'n':
			usenick = true;
			break;
		case 's':
			page = SC;
			break;
		case 'u':
			update = true;
			break;
		case 'v':
			page = VH;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (page) {
			if (update || usenick)
				return CMDNAME + ": cannot use other options "
					"with " + (page == SC ? "-s" : "-v");
			return "[CML] " + CML_HOST + (page == SC
					? CML_SCALC : CML_VHS);
		}
		if (update)
			return CMDNAME + ": must provide RSN to update";
		if (usenick)
			return CMDNAME + ": no Twitch nick given";
		return "[CML] " + CML_HOST;
	} else if (page) {
		return USAGEMSG(CMDNAME, CMDUSAGE);
	}

	/* get the rsn of the queried player */
	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return CMDNAME + ": " + err;
	if (rsn.find(' ') != std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (update) {
		if (updatecml(rsn, err) == 1)
			return output + rsn + "'s CML tracker has been updated";
		else
			return CMDNAME + ": " + err;
	} else {
		return "[CML] " + CML_HOST + CML_USER + rsn;
	}
}

/* updatecml: update the cml tracker of player rsn */
static int updatecml(const std::string &rsn, std::string &err)
{
	int i;
	cpr::Response resp;

	resp = cpr::Get(cpr::Url(CML_HOST + CML_UPDATE + rsn),
			cpr::Header{{ "Connection", "close" }});
	i = std::stoi(resp.text);
	switch (i) {
	case 1:
		/* success */
		break;
	case 2:
		err = "'" + rsn + "' could not be found on the hiscores";
		break;
	case 3:
		err = "'" + rsn + "' has had a negative XP gain";
		break;
	case 4:
		err = "unknown error, try again";
		break;
	case 5:
		err = "'" + rsn + "' has been updated within the last 30s";
		break;
	case 6:
		err = "'" + rsn + "' is an invalid RSN";
		break;
	default:
		err = "could not reach CML API, try again";
		break;
	}
	return i;
}
