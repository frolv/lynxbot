#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
_CMDNAME("delrec");
/* description of the command */
_CMDDESCR("delete a recurring message");
/* command usage synopsis */
_CMDUSAGE("$delrec ID");
/* -a flag usage synposis */
static const char *AUSAGE = "$delrec -a";

/* delrec: delete a recurring message */
std::string CommandHandler::delrec(char *out, struct command *c)
{
	int opt, all;
	int64_t id;
	static struct option long_opts[] = {
		{ "all", NO_ARG, 'a' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	all = 0;
	while ((opt = getopt_long(c->argc, c->argv, "a", long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
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

	if (all) {
		if (optind != c->argc) {
			_USAGEMSG(out, _CMDNAME, AUSAGE);
			return "";
		}
		while (!m_evtp->messages()->empty())
			m_evtp->delMessage(1);
		_sprintf(out, MAX_MSG, "@%s, all recurring message deleted",
				c->nick);
		return "";
	}

	if ((optind != c->argc - 1)) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (!parsenum(c->argv[optind], &id)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s",
				c->argv[0], c->argv[optind]);
		return "";
	}

	if (!m_evtp->delMessage(id))
		_sprintf(out, MAX_MSG, "%s: invalid ID provided", c->argv[0]);
	else
		_sprintf(out, MAX_MSG, "@%s, recurring message %ld deleted.",
				c->nick, id);
	return "";
}
