#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("addcom");
/* description of the command */
CMDDESCR("create a new custom command");
/* command usage synopsis */
CMDUSAGE("$addcom [-c CD] CMD RESPONSE");

static int create(char *out, CustomHandler *cch, struct command *c,
		time_t cooldown);

/* addcom: create a new custom command */
int CmdHandler::addcom(char *out, struct command *c)
{
	time_t cooldown;
	int opt;
	static struct l_option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};


	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	if (!custom_cmds->active()) {
		snprintf(out, MAX_MSG, "%s: custom commands are "
				"currently disabled", c->argv[0]);
		return EXIT_FAILURE;
	}

	cooldown = 15;
	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "c:", long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			/* user provided a cooldown */
			if (!parsenum(l_optarg, &cooldown)) {
				snprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (cooldown < 0) {
				snprintf(out, MAX_MSG, "%s: cooldown cannot be "
						"negative", c->argv[0]);
				return EXIT_FAILURE;
			}
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
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}
	if (l_optind == c->argc - 1) {
		snprintf(out, MAX_MSG, "%s: no response provided for "
				"command $%s", c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}

	return create(out, custom_cmds, c, cooldown);
}

/* create: create a custom command */
static int create(char *out, CustomHandler *cch, struct command *c,
		time_t cooldown)
{
	char *cmd;
	char resp[MAX_MSG];

	cmd = c->argv[l_optind];
	argvcat(resp, c->argc, c->argv, ++l_optind, 1);
	if (!cch->addcom(cmd, resp, c->nick, cooldown)) {
		snprintf(out, MAX_MSG, "%s: %s", c->argv[0], cch->error());
		return EXIT_FAILURE;
	}
	snprintf(out, MAX_MSG, "@%s, command $%s has been added with a %ld"
			"s cooldown.", c->nick, cmd, cooldown);
	return EXIT_SUCCESS;
}
