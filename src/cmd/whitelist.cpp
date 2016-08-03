#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define MAX_URL 256

/* full name of the command */
_CMDNAME("whitelist");
/* description of the command */
_CMDDESCR("exempt websites from moderation");
/* command usage synopsis */
_CMDUSAGE("$whitelist [-d] [SITE]");

/* whitelist: exempt websites from moderation */
std::string CommandHandler::whitelist(char *out, struct command *c)
{
	char url[MAX_URL];
	char *s;
	int del;

	int opt;
	static struct option long_opts[] = {
		{ "delete", NO_ARG, 'd' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	del = 0;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "d", long_opts)) != EOF) {
		switch (opt) {
		case 'd':
			del = 1;
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

	if (optind == c->argc) {
		if (del)
			_sprintf(out, MAX_MSG, "%s: no website specified",
					c->argv[0]);
		else
			_sprintf(out, MAX_MSG, "[WHITELIST] %s",
					m_modp->getFormattedWhitelist().c_str());
		return "";
	}

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
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
			return "";
		}
		if (m_modp->whitelist(url))
			_sprintf(s, MAX_MSG, "%s has beed whitelisted.", url);
		else
			_sprintf(s, MAX_MSG, "%s is already on the whitelist.",
					url);
		return "";
	}

	_sprintf(out, MAX_MSG, "%s: invalid URL: %s", c->argv[0],
			c->argv[optind]);
	return "";
}
