#include <tw/oauth.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("duck");
/* description of the command */
CMDDESCR("search duckduckgo with a query string");
/* command usage synopsis */
CMDUSAGE("$duck QUERY");

static const std::string DDG_QUERY = "https://duckduckgo.com/?q=";

/* duck: search duckduckgo with a query string */
std::string CommandHandler::duck(struct cmdinfo *c)
{
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

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	std::string search = c->fullCmd.substr(op.optind());
	return DDG_QUERY + tw::pencode(search);
}
