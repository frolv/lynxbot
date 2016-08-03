#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("showrec");
/* description of the command */
CMDDESCR("show recurring messages");
/* command usage synopsis */
CMDUSAGE("$showrec [ID]");

/* listrec: show recurring messages */
std::string CommandHandler::showrec(char *out, struct command *c)
{
	int64_t id;
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
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		_sprintf(out, MAX_MSG, "%s", m_evtp->messageList().c_str());
		return "";
	}

	if (optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return "";
	}

	/* show a single message */
	if (!parsenum(c->argv[optind], &id))
		_sprintf(out, MAX_MSG, "%s: invalid number: %s",
				c->argv[0], c->argv[optind]);
	else if (id < 1 || (size_t)id > m_evtp->messages()->size())
		_sprintf(out, MAX_MSG, "%s: recurring message %ld "
				"doesn't exist", c->argv[0], id);
	else
		_sprintf(out, MAX_MSG, "%s", m_evtp->message(id - 1).c_str());
	return "";
}
