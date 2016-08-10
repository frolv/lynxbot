#include <time.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("submit");
/* description of the command */
CMDDESCR("submit a message to the streamer");
/* command usage synopsis */
CMDUSAGE("$submit MSG");

/* submit: submit a message to the streamer */
int CmdHandler::submit(char *out, struct command *c)
{
	char path[MAX_PATH];
	char buf[MAX_MSG];
	time_t t;
	struct tm msgtm;
	FILE *f;

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	_sprintf(path, MAX_PATH, "%s%s", utils::configdir().c_str(),
			utils::config("submit").c_str());
	if (!(f = fopen(path, "a"))) {
		_sprintf(out, MAX_MSG, "%s: could not open submission file",
				c->argv[0]);
		perror(path);
		return EXIT_FAILURE;
	}

	t = time(nullptr);

#ifdef __linux__
	msgtm = *localtime(&t);
#endif
#ifdef _WIN32
	localtime_s(&msgtm, &t);
#endif
	strftime(buf, MAX_MSG, "%Y-%m-%d %R", &msgtm);
	fprintf(f, "[%s] ", buf);
	argvcat(buf, c->argc, c->argv, l_optind, 1);
	fprintf(f, "%s: %s\n", c->nick, buf);
	fclose(f);

	_sprintf(out, MAX_MSG, "@%s, your topic has been submitted. Thank you.",
			c->nick);
	return EXIT_SUCCESS;
}
