#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("delrec");
/* description of the command */
CMDDESCR("delete a recurring message");
/* command usage synopsis */
CMDUSAGE("$delrec [-a] ID");
/* -a flag usage synposis */
static const char *AUSAGE = "$delrec -a";

/* delrec: delete a recurring message */
int CmdHandler::delrec(char *out, struct command *c)
{
	int opt, all;
	int64_t id;
	static struct l_option long_opts[] = {
		{ "all", NO_ARG, 'a' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	all = 0;
	while ((opt = l_getopt_long(c->argc, c->argv, "a", long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
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

	if (all) {
		if (l_optind != c->argc) {
			USAGEMSG(out, CMDNAME, AUSAGE);
			return EXIT_FAILURE;
		}
		while (!evtman->messages()->empty())
			evtman->delmsg(1);
		snprintf(out, MAX_MSG, "@%s, all recurring messages deleted.",
				c->nick);
		return EXIT_SUCCESS;
	}

	if ((l_optind != c->argc - 1)) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (!parsenum(c->argv[l_optind], &id)) {
		snprintf(out, MAX_MSG, "%s: invalid number: %s",
				c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}

	if (!evtman->delmsg(id)) {
		snprintf(out, MAX_MSG, "%s: invalid ID provided", c->argv[0]);
		return EXIT_FAILURE;
	}

	snprintf(out, MAX_MSG, "@%s, recurring message %ld deleted.",
			c->nick, id);
	return EXIT_SUCCESS;
}
