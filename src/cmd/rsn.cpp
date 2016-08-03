#include <algorithm>
#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../RSNList.h"

/* full name of the command */
CMDNAME("rsn");
/* description of the command */
CMDDESCR("view and manage stored rsns");
/* command usage synopsis */
CMDUSAGE("$rsn COMMAND [ARG]");

static int invalidargs(struct command *c);
static void rsn_action(char *out, RSNList *rsns, struct command *c);

/* rsn: view and manage stored rsns */
std::string CommandHandler::rsn(char *out, struct command *c)
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
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (invalidargs(c))
		USAGEMSG(out, CMDNAME, CMDUSAGE);
	else
		rsn_action(out, &m_rsns, c);

	return "";
}

/* invalidargs: check if command arguments are invalid */
static int invalidargs(struct command *c)
{
	return optind == c->argc || (strcmp(c->argv[optind], "set") != 0
			&& strcmp(c->argv[optind], "check") != 0
			&& strcmp(c->argv[optind], "del") != 0
			&& strcmp(c->argv[optind], "change") != 0)
		|| (strcmp(c->argv[optind], "set") == 0 && optind != c->argc - 2)
		|| (strcmp(c->argv[optind], "check") == 0 && optind < c->argc - 2)
		|| (strcmp(c->argv[optind], "del") == 0 && optind != c->argc - 1)
		|| (strcmp(c->argv[optind], "change") == 0 && optind != c->argc - 2);
}

/* rsn_action: perform the requested action */
static void rsn_action(char *out, RSNList *rsns, struct command *c)
{
	const char *crsn;
	char *s, *rsn, *nick;

	if (c->argc == 3) {
		rsn = c->argv[optind + 1];
		for (s = rsn; *s; ++s) {
			*s = tolower(*s);
			if (*s == ' ')
				*s = '_';
		}
	} else {
		rsn = NULL;
	}
	if (strcmp(c->argv[optind], "set") == 0) {
		if (!rsns->add(c->nick, rsn))
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
					rsns->err());
		else
			_sprintf(out, MAX_MSG, "[RSN] The name '%s' has been "
					"set for %s.", rsn, c->nick);
	} else if (strcmp(c->argv[optind], "del") == 0) {
		if (!rsns->del(c->nick))
			_sprintf(out, MAX_MSG, "%s: no RSN set", c->argv[0]);
		else
			_sprintf(out, MAX_MSG, "[RSN] Stored RSN for "
					"%s deleted.", c->nick);
	} else if (strcmp(c->argv[optind], "change") == 0) {
		if (!rsns->edit(c->nick, rsn))
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
					rsns->err());
		else
			_sprintf(out, MAX_MSG, "[RSN] RSN for %s changed to"
					"'%s'.", c->nick, rsn);
	} else {
		/* check own nick or the one that was given */
		nick = c->argc == 2 ? c->nick : rsn;
		if (!(crsn = rsns->rsn(nick)))
			_sprintf(out, MAX_MSG, "[RSN] No RSN set for %s", nick);
		else
			_sprintf(out, MAX_MSG, "[RSN] The RSN '%s' is currently"
					" set for %s.", crsn, nick);
	}
}
