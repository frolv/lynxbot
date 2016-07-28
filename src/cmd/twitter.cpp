#include <tw/reader.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("twitter");
/* description of the command */
CMDDESCR("get information about a twitter user");
/* command usage synopsis */
CMDUSAGE("$twitter [-r] USER");

/* twitter: get information about a twitter user */
std::string CommandHandler::twitter(char *out, struct command *c)
{
	std::string user;
	bool recent;
	tw::Reader reader(m_auth);

	int opt;
	OptionParser op(c->fullCmd, "r");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "recent-tweet", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	recent = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'r':
			recent = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length() ||
			(user = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (!reader.read_user(user))
		return CMDNAME + ": could not read user '" + user + "'";
	if (recent)
		reader.read_recent();

	return "[TWITTER] " + reader.result();
}
