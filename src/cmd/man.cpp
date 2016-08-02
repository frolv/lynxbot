#include <algorithm>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define MAX_URL 128

/* full name of the command */
_CMDNAME("man");
/* description of the command */
_CMDDESCR("view command reference manuals");
/* command usage synopsis */
_CMDUSAGE("$man CMD");

/* man: view command reference manuals */
std::string CommandHandler::man(char *out, struct command *c)
{
	Json::Value *ccmd;
	char url[MAX_URL];

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

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	_sprintf(url, MAX_URL, "%s/manual/", BOT_WEBSITE);
	if (m_help.find(c->argv[optind]) != m_help.end())
		_sprintf(out, MAX_MSG, "[MAN] %s%s.html", url,
				m_help[c->argv[optind]].c_str());
	else if (m_defaultCmds.find(c->argv[optind]) != m_defaultCmds.end())
		_sprintf(out, MAX_MSG, "[MAN] %s%s.html", url, c->argv[optind]);
	else if (!(ccmd = m_customCmds->getcom(c->argv[optind]))->empty())
		_sprintf(out, MAX_MSG, "[MAN] '%s' is a custom command",
				c->argv[optind]);
	else
		_sprintf(out, MAX_MSG, "%s: no manual entry for '%s'",
				c->argv[0], c->argv[optind]);

	return "";
}
