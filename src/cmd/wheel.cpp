#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("selection-wheel");
/* description of the command */
CMDDESCR("select items from various categories");
/* command usage synopsis */
CMDUSAGE("$WHEELCMD CATEGORY");

/* wheel: select items from various categories */
int CmdHandler::wheel(char *out, struct command *c)
{
	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!swheel.active()) {
		snprintf(out, MAX_MSG, "%s: wheel is not currently active",
				c->argv[0]);
		return EXIT_FAILURE;
	}

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
		snprintf(out, MAX_MSG, "%s: %s %s", swheel.name(),
				swheel.desc(), swheel.usage());
		return EXIT_SUCCESS;
	}

	/* check if category is valid */
	if (l_optind != c->argc - 1 || (!swheel.valid((c->argv[l_optind]))
				&& strcmp(c->argv[l_optind], "check") != 0)) {
		USAGEMSG(out, c->argv[0], swheel.usage());
		return EXIT_FAILURE;
	}

	if (strcmp(c->argv[l_optind], "check") == 0) {
		/* return the current selection */
		if (swheel.ready(c->nick))
			snprintf(out, MAX_MSG, "@%s, you are not currently "
					"assigned anything.", c->nick);
		else
			snprintf(out, MAX_MSG, "@%s, you are currently "
					"assigned %s.", c->nick,
					swheel.selection(c->nick));
	} else if (!swheel.ready(c->nick)) {
		snprintf(out, MAX_MSG, "@%s, you have already been "
				"assigned something!", c->nick);
	} else {
		/* make a new selection */
		snprintf(out, MAX_MSG, "@%s, your entertainment for "
				"tonight is %s.", c->nick,
				swheel.choose(c->nick, c->argv[l_optind]));
	}

	return EXIT_SUCCESS;
}
