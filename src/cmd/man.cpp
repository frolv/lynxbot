#include <algorithm>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("man");
/* description of the command */
CMDDESCR("view command reference manuals");
/* command usage synopsis */
CMDUSAGE("$man CMD");

/* man: view command reference manuals */
int CmdHandler::man(char *out, struct command *c)
{
	Json::Value *ccmd;
	char url[MAX_URL];

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

	if (l_optind != c->argc - 1) {
		snprintf(out, MAX_MSG, "What manual page do you want?");
		return EXIT_FAILURE;
	}

	snprintf(url, MAX_URL, BOT_WEBSITE "/manual/");
	if (help.find(c->argv[l_optind]) != help.end())
		snprintf(out, MAX_MSG, "[MAN] %s%s.html", url,
				help[c->argv[l_optind]].c_str());
	else if (default_cmds.find(c->argv[l_optind]) != default_cmds.end())
		snprintf(out, MAX_MSG, "[MAN] %s%s.html", url, c->argv[l_optind]);
	else if ((ccmd = custom_cmds->getcom(c->argv[l_optind])))
		snprintf(out, MAX_MSG, "[MAN] '%s' is a custom command",
				c->argv[l_optind]);
	else
		snprintf(out, MAX_MSG, "%s: no manual entry for '%s'",
				c->argv[0], c->argv[l_optind]);

	return EXIT_SUCCESS;
}
