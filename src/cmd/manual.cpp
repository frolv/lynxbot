#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("manual");
/* description of the command */
CMDDESCR("view the lynxbot manual");
/* command usage synopsis */
CMDUSAGE("$manual");

/* manual: view the lynxbot manual */
std::string CommandHandler::manual(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc)
		USAGEMSG(out, CMDNAME, CMDUSAGE);
	else
		_sprintf(out, MAX_MSG, "[MANUAL] %s/manual/index.html",
				BOT_WEBSITE);
	return "";
}
