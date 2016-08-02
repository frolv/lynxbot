#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("setrec");
/* description of the command */
_CMDDESCR("enable/disabe recurring messages");
/* command usage synopsis */
_CMDUSAGE("$setrec on|off");

/* setrec: enable and disable recurring messages */
std::string CommandHandler::setrec(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

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

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
	} else if (strcmp(c->argv[optind], "on") == 0) {
		m_evtp->activateMessages();
		_sprintf(out, MAX_MSG, "@%s, recurring mesasges enabled.",
				c->nick);
	} else if (strcmp(c->argv[optind], "off") == 0) {
		m_evtp->deactivateMessages();
		_sprintf(out, MAX_MSG, "@%s, recurring mesasges disabled.",
				c->nick);
	} else {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
	}
	return "";
}
