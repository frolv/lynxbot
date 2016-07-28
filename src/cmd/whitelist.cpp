#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("whitelist");
/* description of the command */
CMDDESCR("exempt websites from moderation");
/* command usage synopsis */
CMDUSAGE("$whitelist [-d] [SITE]");

/* whitelist: exempt websites from moderation */
std::string CommandHandler::whitelist(char *out, struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string website, outp;
	bool del;

	int opt;
	OptionParser op(c->fullCmd, "d");
	static struct OptionParser::option long_opts[] = {
		{ "delete", NO_ARG, 'd' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	del = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'd':
			del = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (del)
			return CMDNAME + ": no website specified";
		/* no args: show current whitelist */
		return m_modp->getFormattedWhitelist();
	}

	if ((website = c->fullCmd.substr(op.optind())).find(' ')
				!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	outp = "@" + std::string(c->nick) + ", ";
	if (m_parsep->parse(website)) {
		/* extract domain and add to whitelist */
		website = m_parsep->getLast()->subdomain
			+ m_parsep->getLast()->domain;
		if (del) {
			if (m_modp->delurl(website))
				return outp + website + " has been "
					"removed from the whitelist.";
			else
				return outp + website + " is not on "
					"the whitelist.";
		}
		if (m_modp->whitelist(website))
			return outp + website + " has been whitelisted.";
		else
			return outp + website + " is already on the whitelist.";
	}

	return CMDNAME + ": invalid URL: " + website;
}
