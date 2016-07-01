#include <cpr/cpr.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("cml");
/* description of the command */
CMDDESCR("interact with crystalmathlabs trackers");
/* command usage synopsis */
CMDUSAGE("$cml [-nu] [RSN]");

static const std::string CML_HOST = "crystalmathlabs.com";
static const std::string CML_UPDATE = "/tracker/api.php?type=update&player=";

static int updatecml(const std::string &rsn, std::string &err);

/* cml: interact with crystalmathlabs trackers */
std::string CommandHandler::cml(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	std::string rsn, err;
	bool usenick, update;

	OptionParser op(c->fullCmd, "nu");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "nick", NO_ARG, 'n' },
		{ "update", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	usenick = update = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'n':
			usenick = true;
			break;
		case 'u':
			update = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (update)
			return CMDNAME + ": must provide RSN to update";
		if (usenick)
			return CMDNAME + ": no Twitch nick given";
		else
			return "[CML] http://" + CML_HOST;
	}

	/* get the rsn of the queried player */
	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return err;
	if (rsn.find(' ') != std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (update) {
		if (updatecml(rsn, err) == 1)
			return output + rsn + "'s CML has been updated";
		else
			return CMDNAME + ": " + err;
	} else {
		return "[CML] http://" + CML_HOST + CML_USER + rsn;
	}
}

/* updatecml: update the cml tracker of player rsn */
static int updatecml(const std::string &rsn, std::string &err)
{
	int i;
	cpr::Response resp;

	resp = cpr::Get(cpr::Url("http://" + CML_HOST + CML_UPDATE + rsn),
			cpr::Header{{ "Connection", "close" }});
	i = std::stoi(resp.text);
	switch (i) {
	case 1:
		/* success */
		break;
	case 2:
		err = rsn + " could not be found on the hiscores";
	case 3:
		err = rsn + " has had a negative XP gain";
	case 4:
		err = "unknown error, try again";
	case 5:
		err = rsn + " has been updated within the last 30s";
	case 6:
		err = rsn + " is an invalid RSN";
	default:
		err = "could not reach CML API, try again";
	}
	return i;
}
