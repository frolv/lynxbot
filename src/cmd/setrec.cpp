#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("setrec");
/* description of the command */
CMDDESCR("enable/disabe recurring messages");
/* command usage synopsis */
CMDUSAGE("$setrec on|off");

/* setrec: enable and disable recurring messages */
int CmdHandler::setrec(char *out, struct command *c)
{
	int opt, status;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

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

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		status = EXIT_FAILURE;
	} else if (strcmp(c->argv[l_optind], "on") == 0) {
		m_evtp->activate();
		snprintf(out, MAX_MSG, "@%s, recurring mesasges enabled.",
				c->nick);
	} else if (strcmp(c->argv[l_optind], "off") == 0) {
		m_evtp->deactivate();
		snprintf(out, MAX_MSG, "@%s, recurring mesasges disabled.",
				c->nick);
	} else {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		status = EXIT_FAILURE;
	}
	return status;
}
