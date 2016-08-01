#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("8ball");
/* description of the command */
_CMDDESCR("respond to questions");
/* command usage synopsis */
_CMDUSAGE("$8ball QUESTION");

/* 8ball: respond to questions */
std::string CommandHandler::eightball(char *out, struct command *c)
{
	int opt;
	char *last;
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

	last = c->argv[c->argc - 1];
	last += strlen(last) - 1;
	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
	} else if (*last != '?') {
		_sprintf(out, MAX_MSG, "[8 BALL] Ask me a question.");
	} else {
		std::uniform_int_distribution<> dis(0, m_eightball.size());
		_sprintf(out, MAX_MSG, "[8 BALL] @%s, %s.", c->nick,
				m_eightball[dis(m_gen)].c_str());
	}
	return "";
}
