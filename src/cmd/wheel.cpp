#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("selection-wheel");
/* description of the command */
_CMDDESCR("select items from various categories");
/* command usage synopsis */
_CMDUSAGE("$WHEELCMD CATEGORY");

/* wheel: select items from various categories */
std::string CommandHandler::wheel(char *out, struct command *c)
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
		_sprintf(out, MAX_MSG, "%s: %s %s", m_wheel.name(),
				m_wheel.desc(), m_wheel.usage());
		return "";
	}

	/* check if category is valid */
	if (optind != c->argc - 1 || (!m_wheel.valid((c->argv[optind]))
				&& strcmp(c->argv[optind], "check") != 0)) {
		_USAGEMSG(out, c->argv[0], m_wheel.usage());
		return "";
	}

	if (strcmp(c->argv[optind], "check") == 0) {
		/* return the current selection */
		if (m_wheel.ready(c->nick))
			_sprintf(out, MAX_MSG, "@%s, you are not currently "
					"assigned anything.", c->nick);
		else
			_sprintf(out, MAX_MSG, "@%s, you are currently "
					"assigned %s.", c->nick,
					m_wheel.selection(c->nick));
	} else if (!m_wheel.ready(c->nick)) {
		_sprintf(out, MAX_MSG, "@%s, you have already been "
				"assigned something!", c->nick);
	} else {
		/* make a new selection */
		_sprintf(out, MAX_MSG, "@%s, for entertainment for "
				"tonight is %s.", c->nick,
				m_wheel.choose(c->nick, c->argv[optind]));
	}

	return "";
}
