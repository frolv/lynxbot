#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
_CMDNAME("permit");
/* description of the command */
_CMDDESCR("grant user permission to post urls");
/* command usage synopsis */
_CMDUSAGE("$permit [-n AMT] [-s] USER");

/* permit: grant user permission to post a url */
std::string CommandHandler::permit(char *out, struct command *c)
{
	int opt;
	int64_t amt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "amount", REQ_ARG, 'n' },
		{ "session", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	amt = 1;
	while ((opt = getopt_long(c->argc, c->argv, "n:s", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'n':
			if (!parsenum(optarg, &amt)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], optarg);
				return "";
			}
			if (amt < 1) {
				_sprintf(out, MAX_MSG, "%s: amount must be a "
						"positive integer", c->argv[0]);
				return "";
			}
			break;
		case 's':
			amt = -1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	m_modp->permit(c->argv[optind], amt);
	if (amt == -1)
		_sprintf(out, MAX_MSG, "[PERMIT] %s has been granted permission"
				" to post links for the duration of this "
				"session.", c->argv[optind]);
	else
		_sprintf(out, MAX_MSG, "[PERMIT] %s has been granted permission"
				" to post %ld link%s.", c->argv[optind], amt,
				amt == 1 ? "" : "s");
	return "";
}
