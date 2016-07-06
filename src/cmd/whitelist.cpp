#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("whitelist");
/* description of the command */
CMDDESCR("exempt websites from moderation");
/* command usage synopsis */
CMDUSAGE("$whitelist [SITE]");

/* whitelist: exempt websites from moderation */
std::string CommandHandler::whitelist(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string website;
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

	/* no args: show current whitelist */
	if (op.optind() == c->fullCmd.length())
		return m_modp->getFormattedWhitelist();

	if ((website = c->fullCmd.substr(op.optind())).find(' ')
				!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (m_parsep->parse(website)) {
		/* extract domain and add to whitelist */
		website = m_parsep->getLast()->subdomain
			+ m_parsep->getLast()->domain;
		if (m_modp->whitelist(website))
			return "@" + c->nick + ", " + website
				+ " has been whitelisted.";
		else
			return "@" + c->nick + ", " + website
				+ " is already on the whitelist.";
	}

	return CMDNAME + ": invalid URL: " + website;
}
