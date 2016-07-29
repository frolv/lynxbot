#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("permit");
/* description of the command */
CMDDESCR("grant user permission to post urls");
/* command usage synopsis */
CMDUSAGE("$permit [-n AMT] [-s] USER");

/* permit: grant user permission to post a url */
std::string CommandHandler::permit(char *out, struct command *c)
{
	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	std::string nick;
	int opt, amt;
	OptionParser op(c->fullCmd, "n:s");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "amount", REQ_ARG, 'n' },
		{ "session", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	amt = 1;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'n':
			try {
				if ((amt = std::stoi(op.optarg())) < 1)
					return CMDNAME + ": amount must be a "
						"positive integer";
			} catch (std::invalid_argument) {
				return CMDNAME + ": invalid number -- '"
					+ op.optarg() + "'";
			} catch (std::out_of_range) {
				return CMDNAME + ": number too large";
			}
			break;
		case 's':
			amt = -1;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length() ||
			(nick = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	m_modp->permit(nick, amt);
	if (amt == -1)
		return "[PERMIT] " + nick + " has been granted permission "
			" to post links for the duration of this session.";

	return "[PERMIT] " + nick + " has been granted permission "
		"to post " + std::to_string(amt) + " link"
		+ (amt == 1 ? "" : "s") + ".";
}
