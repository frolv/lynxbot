#include <tw/reader.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("twitter");
/* description of the command */
CMDDESCR("get information about a twitter user");
/* command usage synopsis */
CMDUSAGE("$twitter [-r] USER");

/* twitter: get information about a twitter user */
int CmdHandler::twitter(char *out, struct command *c)
{
	int recent;
	tw::Reader reader(tw_auth);

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "recent-tweet", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	recent = 0;
	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "r", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		case 'r':
			recent = 1;
			break;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (!reader.read_user(c->argv[l_optind])) {
		snprintf(out, MAX_MSG, "%s: could not read user '%s'",
				c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}
	if (recent)
		reader.read_recent();

	snprintf(out, MAX_MSG, "[TWITTER] %s", reader.result().c_str());
	return EXIT_SUCCESS;
}
