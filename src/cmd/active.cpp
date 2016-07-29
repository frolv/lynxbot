#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("active");
/* description of the command */
_CMDDESCR("view current poll");
/* command usage synopsis */
_CMDUSAGE("$active");

static const char *SP_HOST = "https://strawpoll.me";

/* active: view current poll */
std::string CommandHandler::active(char *out, struct command *c)
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

	if (optind != c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (!*m_poll)
		_sprintf(out, MAX_MSG, "[ACTIVE] no poll has been created");
	else
		_sprintf(out, MAX_MSG, "[ACTIVE] %s/%s", SP_HOST, m_poll);
	return "";
}
