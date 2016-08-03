#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("addcom");
/* description of the command */
CMDDESCR("create a new custom command");
/* command usage synopsis */
CMDUSAGE("$addcom [-c CD] CMD RESPONSE");

static void create(char *out, CustomCommandHandler *cch,
		struct command *c, time_t cooldown);

/* addcom: create a new custom command */
std::string CommandHandler::addcom(char *out, struct command *c)
{
	time_t cooldown;
	int opt;
	static struct option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};


	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	if (!m_customCmds->isActive()) {
		_sprintf(out, MAX_MSG, "%s: custom commands are "
				"currently disabled", c->argv[0]);
		return "";
	}

	cooldown = 15;
	opt_init();
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
			break;
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
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return "";
	}
	if (optind == c->argc - 1) {
		_sprintf(out, MAX_MSG, "%s: no response provided for "
				"command $%s", c->argv[0], c->argv[optind]);
		return "";
	}

	create(out, m_customCmds, c, cooldown);
	return "";
}

/* create: create a custom command */
static void create(char *out, CustomCommandHandler *cch,
		struct command *c, time_t cooldown)
{
	char *cmd;
	char resp[MAX_MSG];

	cmd = c->argv[optind];
	argvcat(resp, c->argc, c->argv, ++optind, 1);
	if (!cch->addcom(cmd, resp, c->nick, cooldown)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
		return;
	}
	_sprintf(out, MAX_MSG, "@%s, command $%s has been added with a %ld"
			"s cooldown", c->nick, cmd, cooldown);
}
