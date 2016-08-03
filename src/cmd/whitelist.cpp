#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

#define MAX_URL 256

/* full name of the command */
CMDNAME("whitelist");
/* description of the command */
CMDDESCR("exempt websites from moderation");
/* command usage synopsis */
CMDUSAGE("$whitelist [-d] [SITE]");

/* whitelist: exempt websites from moderation */
int CmdHandler::whitelist(char *out, struct command *c)
{
	char url[MAX_URL];
	char *s;
	int del, status;

	int opt;
	static struct option long_opts[] = {
		{ "delete", NO_ARG, 'd' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	del = 0;
	status = EXIT_SUCCESS;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "d", long_opts)) != EOF) {
		switch (opt) {
		case 'd':
			del = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (optind == c->argc) {
		if (del) {
			_sprintf(out, MAX_MSG, "%s: no website specified",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			_sprintf(out, MAX_MSG, "[WHITELIST] %s",
					m_modp->getFormattedWhitelist().c_str());
		}
		return status;
	}

	if (optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	_sprintf(out, MAX_MSG, "@%s, ", c->nick);
	s = strchr(out, '\0');
	if (m_parsep->parse(c->argv[optind])) {
		/* extract domain and add to whitelist */
		_sprintf(url, MAX_URL, "%s%s",
				m_parsep->getLast()->subdomain.c_str(),
				m_parsep->getLast()->domain.c_str());
		if (del) {
			if (m_modp->delurl(url))
				_sprintf(s, MAX_MSG, "%s has been removed "
						"from the whitelist.", url);
			else
				_sprintf(s, MAX_MSG, "%s is not on the "
						"whitelist.", url);
			return status;
		}
		if (m_modp->whitelist(url))
			_sprintf(s, MAX_MSG, "%s has beed whitelisted.", url);
		else
			_sprintf(s, MAX_MSG, "%s is already on the whitelist.",
					url);
		return status;
	}

	_sprintf(out, MAX_MSG, "%s: invalid URL: %s", c->argv[0],
			c->argv[optind]);
	return EXIT_FAILURE;
}
