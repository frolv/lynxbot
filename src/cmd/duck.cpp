#include <tw/oauth.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("duck");
/* description of the command */
_CMDDESCR("search duckduckgo with a query string");
/* command usage synopsis */
_CMDUSAGE("$duck QUERY");

static const char *DDG_QUERY = "https://duckduckgo.com/?q=";

/* duck: search duckduckgo with a query string */
std::string CommandHandler::duck(char *out, struct command *c)
{
	char query[MAX_MSG];
	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	argvcat(query, c->argc, c->argv, optind, 1);
	_sprintf(out, MAX_MSG, "%s%s", DDG_QUERY, tw::pencode(query).c_str());
	return "";
}
