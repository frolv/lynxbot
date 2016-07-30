#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
_CMDNAME("addrec");
/* description of the command */
_CMDDESCR("add a recurring message");
/* command usage synopsis */
_CMDUSAGE("$addrec [-c INT] MSG");

/* addrec: add a recurring message */
std::string CommandHandler::addrec(char *out, struct command *c)
{
	time_t cooldown;
	char msg[MAX_MSG];

	int opt;
	static struct option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "interval", REQ_ARG, 'c' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	cooldown = 300;
	while ((opt = getopt_long(c->argc, c->argv, "c:", long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			/* user provided a cooldown */
			if (!parsenum(optarg, &cooldown)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], optarg);
				return "";
			}
			if (cooldown < 0) {
				_sprintf(out, MAX_MSG, "%s: cooldown cannot be "
						"negative", c->argv[0]);
				return "";
			}
			cooldown *= 60;
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

	if (cooldown % 300 != 0) {
		_sprintf(out, MAX_MSG, "%s: interval must be a multiple "
				"of 5 mins", c->argv[0]);
		return "";
	}
	else if (cooldown > 3600) {
		_sprintf(out, MAX_MSG, "%s: interval cannot be longer "
				"than 60 mins", c->argv[0]);
		return "";
	}
	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	/* build the message */
	msg[0] = '\0';
	for (; optind < c->argc; ++optind) {
		strcat(msg, c->argv[optind]);
		if (optind != c->argc - 1)
			strcat(msg, " ");
	}

	if (!m_evtp->addMessage(msg, cooldown))
		_sprintf(out, MAX_MSG, "%s: limit of 5 recurring "
				"messages reached", c->argv[0]);
	else
		_sprintf(out, MAX_MSG, "@%s, recurring message \"%s\" has been "
				"added at a %ld min interval", c->nick, msg,
				cooldown / 60);
	return "";
}
