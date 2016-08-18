#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("permit");
/* description of the command */
CMDDESCR("grant user permission to post urls");
/* command usage synopsis */
CMDUSAGE("$permit [-n AMT] [-s] USER");

/* permit: grant user permission to post a url */
int CmdHandler::permit(char *out, struct command *c)
{
	int opt;
	int64_t amt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "amount", REQ_ARG, 'n' },
		{ "session", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	amt = 1;
	while ((opt = l_getopt_long(c->argc, c->argv, "n:s", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'n':
			if (!parsenum(l_optarg, &amt)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (amt < 1) {
				_sprintf(out, MAX_MSG, "%s: amount must be a "
						"positive integer", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 's':
			amt = -1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (!m_modp->permit(c->argv[l_optind], amt)) {
		_sprintf(out, MAX_MSG, "%s: user '%s' is not currently in the "
				"channel", c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}

	if (amt == -1)
		_sprintf(out, MAX_MSG, "[PERMIT] %s has been granted permission"
				" to post links for the duration of this "
				"session.", c->argv[l_optind]);
	else
		_sprintf(out, MAX_MSG, "[PERMIT] %s has been granted permission"
				" to post %ld link%s.", c->argv[l_optind], amt,
				amt == 1 ? "" : "s");
	return EXIT_SUCCESS;
}
