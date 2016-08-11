#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../sed.h"
#include "../stringparse.h"

#define ON  1
#define OFF 2

/* full name of the command */
CMDNAME("editcom");
/* description of the command */
CMDDESCR("modify a custom command");
/* command usage synopsis */
CMDUSAGE("$editcom [-A on|off] [-a] [-c CD] [-r] [-s SEDCMD] CMD [RESPONSE]");
/* rename flag usage */
static const char *RUSAGE = "$editcom -r OLD NEW";

/* whether to append to existing response */
static int app;
/* active setting */
static int set;
/* command cooldown */
static time_t cooldown;

static int edit(char *out, CustomHandler *cch, struct command *c, char *resp);
static int rename(char *out, CustomHandler *cch, struct command *c);
static void outputmsg(char *out, struct command *c, char *resp);

/* editcom: modify a custom command */
int CmdHandler::editcom(char *out, struct command *c)
{
	int ren, status;
	char *sedcmd;
	char buf[MAX_MSG];
	char response[MAX_MSG];
	Json::Value *com;

	int opt;
	static struct l_option long_opts[] = {
		{ "active", REQ_ARG, 'A'},
		{ "append", NO_ARG, 'a'},
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ "rename", NO_ARG, 'r' },
		{ "sed", REQ_ARG, 's' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	if (!m_customCmds->isActive()) {
		_sprintf(out, MAX_MSG, "%s: custom commands are currently "
				"disabled", c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	cooldown = -1;
	set = ren = app = 0;
	sedcmd = NULL;
	status = EXIT_SUCCESS;
	while ((opt = l_getopt_long(c->argc, c->argv, "A:ac:rs:", long_opts))
			!= EOF) {
		switch (opt) {
		case 'A':
			if (strcmp(l_optarg, "on") == 0) {
				set = ON;
			} else if (strcmp(l_optarg, "off") == 0) {
				set = OFF;
			} else {
				_sprintf(out, MAX_MSG, "%s: -A setting must "
						"be on/off", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'a':
			app = 1;
			break;
		case 'c':
			/* user provided a cooldown */
			if (!parsenum(l_optarg, &cooldown)) {
				_sprintf(out, MAX_MSG, "%s: invalid number: %s",
						c->argv[0], l_optarg);
				return EXIT_FAILURE;
			}
			if (cooldown < 0) {
				_sprintf(out, MAX_MSG, "%s: cooldown cannot be "
						"negative", c->argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'r':
			ren = 1;
			break;
		case 's':
			sedcmd = l_optarg;
			break;
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

	if ((com = m_customCmds->getcom(c->argv[l_optind]))->empty()) {
		_sprintf(out, MAX_MSG, "%s: not a command: $%s",
				c->argv[0], c->argv[l_optind]);
		return EXIT_FAILURE;
	}

	if (sedcmd) {
		if (app || ren) {
			_sprintf(out, MAX_MSG, "%s: cannot use -a or -r "
					"with -s", c->argv[0]);
			status = EXIT_FAILURE;
		} else if (l_optind != c->argc - 1) {
			_sprintf(out, MAX_MSG, "%s: cannot provide reponse"
					"with -s flag", c->argv[0]);
			status = EXIT_FAILURE;
		} else if (!sed(response, MAX_MSG, (*com)["response"]
					.asCString(), sedcmd)) {
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], response);
			status = EXIT_FAILURE;
		}
		if (status)
			return status;
	} else if (ren) {
		if (app || cooldown != -1 || set || sedcmd) {
			_sprintf(out, MAX_MSG, "%s: cannot use other flags "
					"with -r", c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			status = rename(out, m_customCmds, c);
		}
		return status;
	} else {
		argvcat(response, c->argc, c->argv, l_optind + 1, 1);
		if (app) {
			strcpy(buf, response);
			_sprintf(response, MAX_MSG, "%s %s",
					(*com)["response"].asCString(), buf);
		}
	}
	return edit(out, m_customCmds, c, *response ? response : NULL);
}

/* edit: edit a custom command */
static int edit(char *out, CustomHandler *cch, struct command *c, char *resp)
{
	if (!cch->editcom(c->argv[l_optind], resp, cooldown)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
		return EXIT_FAILURE;
	}
	if (cooldown == -1 && !resp && !set) {
		_sprintf(out, MAX_MSG, "@%s, command $%s was unchanged",
				c->nick, c->argv[l_optind]);
		return EXIT_SUCCESS;
	}
	if (set == ON && !cch->activate(c->argv[l_optind])) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
		return EXIT_FAILURE;
	} else if (set == OFF) {
		cch->deactivate(c->argv[l_optind]);
	}
	outputmsg(out, c, resp);
	return EXIT_SUCCESS;
}

/* rename: rename a custom command */
static int rename(char *out, CustomHandler *cch, struct command *c)
{
	int status;

	if (l_optind != c->argc - 2) {
		USAGEMSG(out, CMDNAME, RUSAGE);
		status = EXIT_FAILURE;
	} else if (!cch->rename(c->argv[l_optind], c->argv[l_optind + 1])) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
		status = EXIT_FAILURE;
	} else {
		_sprintf(out, MAX_MSG, "@%s, command $%s has been renamed "
				"to $%s", c->nick, c->argv[l_optind],
				c->argv[l_optind + 1]);
		status = EXIT_SUCCESS;
	}
	return status;
}

/* outputmsg: write a message to out detailing command changes */
static void outputmsg(char *out, struct command *c, char *resp)
{
	int cd;

	cd = cooldown != -1;
	_sprintf(out, MAX_MSG, "@%s, command $%s has been",
			c->nick, c->argv[l_optind]);
	if (set == ON)
		strcat(out, " activated");
	else if (set == OFF)
		strcat(out, " deactivated");

	if (resp || cd) {
		if (set)
			strcat(out, " and");
		strcat(out, " changed to ");
		out = strchr(out, '\0');
		if (resp) {
			_sprintf(out, MAX_MSG, "\"%s\"%s", resp,
					cd ? ", with " : "");
			out = strchr(out, '\0');
		}
		if (cd) {
			_sprintf(out, MAX_MSG, "a %lds cooldown", cooldown);
		}
	}
	strcat(out, ".");
}
