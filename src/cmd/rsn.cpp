#include <algorithm>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"
#include "../RSNList.h"

/* full name of the command */
CMDNAME("rsn");
/* description of the command */
CMDDESCR("view and manage stored rsns");
/* command usage synopsis */
CMDUSAGE("$rsn COMMAND [ARG]");

static std::string rsn_action(RSNList *rsns, const std::string user,
		const std::vector<std::string> &argv);

/* rsn: view and manage stored rsns */
std::string CommandHandler::rsn(char *out, struct command *c)
{
	std::vector<std::string> argv;

	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	utils::split(c->fullCmd.substr(op.optind()), ' ', argv);
	if (argv.size() < 1 || (argv[0] != "set" && argv[0] != "check"
				&& argv[0] != "del" && argv[0] != "change")
			|| (argv[0] == "set" && argv.size() != 2)
			|| (argv[0] == "check" && argv.size() > 2)
			|| (argv[0] == "del" && argv.size() != 1)
			|| (argv[0] == "change" && argv.size() != 2))
		return USAGEMSG(CMDNAME, CMDUSAGE);

	return rsn_action(&m_rsns, c->nick, argv);
}

/* rsn_action: perform the requested action */
static std::string rsn_action(RSNList *rsns, const std::string user,
		const std::vector<std::string> &argv)
{
	const char *crsn;
	std::string err, rsn, nick;

	if (argv.size() > 1) {
		rsn = argv[1];
		std::transform(rsn.begin(), rsn.end(), rsn.begin(), tolower);
	}
	if (argv[0] == "set") {
		if (!rsns->add(user, rsn, err))
			return CMDNAME + ": " + err;
		else
			return "RSN " + rsn + " has been set for "
				+ user + ".";
	} else if (argv[0] == "del") {
		if (!rsns->del(user))
			return CMDNAME + ": no rsn set";
		else
			return "RSN for " + user + " has been deleted.";
	} else if (argv[0] == "change") {
		if (!rsns->edit(user, rsn, err))
			return CMDNAME + ": " + err;
		else
			return "RSN for " + user + " changed to "
				+ rsn + ".";
	} else {
		/* check own nick or the one that was given */
		nick = argv.size() == 1 ? user : rsn;
		if (!(crsn = rsns->getRSN(nick.c_str())))
			return "No RSN set for " + nick + ".";
		else
			return "RSN '" + std::string(crsn) + "' is currently set for "
				+ nick + ".";
	}
}
