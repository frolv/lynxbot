#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("8ball");
/* description of the command */
CMDDESCR("respond to questions");
/* command usage synopsis */
CMDUSAGE("$8ball QUESTION");

/* 8ball: respond to questions */
int CmdHandler::eightball(char *out, struct command *c)
{
	int opt, status;
	char *last;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	status = EXIT_SUCCESS;
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

	last = c->argv[c->argc - 1];
	last += strlen(last) - 1;
	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		status = EXIT_FAILURE;
	} else if (*last != '?') {
		snprintf(out, MAX_MSG, "[8 BALL] Ask me a question.");
		status = EXIT_FAILURE;
	} else {
		std::uniform_int_distribution<> dis(
				0, eightball_responses.size());
		snprintf(out, MAX_MSG, "[8 BALL] @%s, %s.", c->nick,
				eightball_responses[dis(gen)].c_str());
	}
	return status;
}
