#include <tw/reader.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("twitter");
/* description of the command */
_CMDDESCR("get information about a twitter user");
/* command usage synopsis */
_CMDUSAGE("$twitter [-r] USER");

/* twitter: get information about a twitter user */
std::string CommandHandler::twitter(char *out, struct command *c)
{
	int recent;
	tw::Reader reader(m_auth);

	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "recent-tweet", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	recent = 0;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "r", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		case 'r':
			recent = 1;
			break;
		default:
			return "";
		}
	}

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (!reader.read_user(c->argv[optind])) {
		_sprintf(out, MAX_MSG, "%s: could not read user '%s'",
				c->argv[0], c->argv[optind]);
		return "";
	}
	if (recent)
		reader.read_recent();

	_sprintf(out, MAX_MSG, "[TWITTER] %s", reader.result().c_str());
	return "";
}
