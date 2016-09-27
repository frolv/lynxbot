#include <tw/oauth.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("duck");
/* description of the command */
CMDDESCR("search duckduckgo with a query string");
/* command usage synopsis */
CMDUSAGE("$duck QUERY");

static const char *DDG_QUERY = "https://duckduckgo.com/?q=";

/* duck: search duckduckgo with a query string */
int CmdHandler::duck(char *out, struct command *c)
{
	char query[MAX_MSG];
	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	argvcat(query, c->argc, c->argv, l_optind, 1);
	snprintf(out, MAX_MSG, "[DUCK] %s%s", DDG_QUERY, tw::pencode(query).c_str());
	return EXIT_SUCCESS;
}
