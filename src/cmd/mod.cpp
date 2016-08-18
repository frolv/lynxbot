#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("mod");
/* description of the command */
CMDDESCR("perform moderation commands");
/* command usage synopsis */
CMDUSAGE("$mod <-b|-t> USER");

/* mod: perform moderation commands */
int CmdHandler::mod(char *out, struct command *c)
{
	char *s;
	const char *t;
	int action;
	int64_t len;
	char reason[MAX_MSG];
	char name[RSN_BUF];

	int opt;
	static struct l_option long_opts[] = {
		{ "ban", NO_ARG, 'b' },
		{ "help", NO_ARG, 'h' },
		{ "length", REQ_ARG, 'l' },
		{ "timeout", NO_ARG, 't' },
		{ 0, 0, 0 }
	};

	opt_init();
	action = 0;
	len = 300;
	while ((opt = l_getopt_long(c->argc, c->argv, "bl:t", long_opts))
			!= EOF) {
		switch (opt) {
		case 'b':
			if (action) {
				_sprintf(out, MAX_MSG, "%s: cannot both timeout"
						" and ban", c->argv[0]);
				return EXIT_FAILURE;
			}
			action = BAN;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'l':
			if (!parsenum(l_optarg, &len)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (len < 0) {
				_sprintf(out, MAX_MSG, "%s: length cannot be "
						"negative", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 't':
			if (action) {
				_sprintf(out, MAX_MSG, "%s: cannot both timeout"
						" and ban", c->argv[0]);
				return EXIT_FAILURE;
			}
			action = TIMEOUT;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (!action || l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	/* convert names to lower for lookup */
	s = name;
	t = m_name;
	while ((*s++ = tolower(*t++)))
		;
	for (s = c->argv[l_optind]; *s; ++s)
		*s = tolower(*s);

	if (strcmp(c->argv[l_optind], name) == 0) {
		_sprintf(out, MAX_MSG, "@%s, I'd rather not.", c->nick);
		return EXIT_SUCCESS;
	}
	if (strcmp(c->argv[l_optind], c->nick) == 0) {
		_sprintf(out, MAX_MSG, "@%s, don't be silly!", c->nick);
		return EXIT_SUCCESS;
	}

	argvcat(reason, c->argc, c->argv, l_optind + 1, 1);
	/* attempt to perform the moderation action */
	if (!m_modp->mod_action(action, c->argv[l_optind],
				c->nick, reason, len)) {
		_sprintf(out, MAX_MSG, "%s: user '%s' is not currently in the "
				"channel", c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}

	_sprintf(out, MAX_MSG, "@%s, user '%s' has been %s for %s.",
			c->nick, c->argv[l_optind],
			action == BAN ? "banned" : "timed out",
			reason);
	return EXIT_SUCCESS;
}
