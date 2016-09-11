#include <string.h>
#include <utils.h>
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
	static struct l_option long_opts[] = {
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
	while ((opt = l_getopt_long(c->argc, c->argv, "d", long_opts)) != EOF) {
		switch (opt) {
		case 'd':
			del = 1;
			break;
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
		if (del) {
			snprintf(out, MAX_MSG, "%s: no website specified",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else if (moderator->paste()) {
			snprintf(out, MAX_MSG, "[WHITELIST] %s",
					utils::upload(moderator->
						fmt_whitelist()).c_str());
		} else {
			snprintf(out, MAX_MSG, "[WHITELIST] %s",
					moderator->fmt_whitelist().c_str());
		}
		return status;
	}

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	snprintf(out, MAX_MSG, "@%s, ", c->nick);
	s = strchr(out, '\0');
	if (urlparser->parse(c->argv[l_optind])) {
		/* extract domain and add to whitelist */
		snprintf(url, MAX_URL, "%s%s",
				urlparser->getLast()->subdomain.c_str(),
				urlparser->getLast()->domain.c_str());
		if (del) {
			if (moderator->delurl(url))
				snprintf(s, MAX_MSG, "%s has been removed "
						"from the whitelist.", url);
			else
				snprintf(s, MAX_MSG, "%s is not on the "
						"whitelist.", url);
			return status;
		}
		if (moderator->whitelist(url))
			snprintf(s, MAX_MSG, "%s has beed whitelisted.", url);
		else
			snprintf(s, MAX_MSG, "%s is already on the whitelist.",
					url);
		return status;
	}

	snprintf(out, MAX_MSG, "%s: invalid URL: %s", c->argv[0],
			c->argv[l_optind]);
	return EXIT_FAILURE;
}
