#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("manual");
/* description of the command */
CMDDESCR("view the lynxbot manual");
/* command usage synopsis */
CMDUSAGE("$manual");

/* manual: view the lynxbot manual */
int CmdHandler::manual(char *out, struct command *c)
{
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
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	_sprintf(out, MAX_MSG, "[MANUAL] %s/manual/index.html", BOT_WEBSITE);
	return EXIT_SUCCESS;
}
