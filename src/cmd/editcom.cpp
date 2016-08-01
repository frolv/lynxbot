#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../stringparse.h"
#include "../TimerManager.h"

#define ON  1
#define OFF 2

/* full name of the command */
_CMDNAME("editcom");
/* description of the command */
_CMDDESCR("modify a custom command");
/* command usage synopsis */
_CMDUSAGE("$editcom [-A on|off] [-a] [-c CD] [-r] CMD [RESPONSE]");
/* rename flag usage */
static const char *RUSAGE = "$editcom -r OLD NEW";

/* whether to append to existing response */
static int app;
/* active setting */
static int set;

static void edit(char *out, CustomCommandHandler *cch, struct command *c,
		time_t cooldown, TimerManager *tm);
static void rename(char *out, CustomCommandHandler *cch, struct command *c);

/* editcom: modify a custom command */
std::string CommandHandler::editcom(char *out, struct command *c)
{
	time_t cooldown;
	int ren;

	int opt;
	static struct option long_opts[] = {
		{ "active", REQ_ARG, 'A'},
		{ "append", NO_ARG, 'a'},
		{ "cooldown", REQ_ARG, 'c'},
		{ "help", NO_ARG, 'h' },
		{ "rename", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	if (!m_customCmds->isActive()) {
		_sprintf(out, MAX_MSG, "%s: custom commands are currently "
				"disabled", c->argv[0]);
		return "";
	}

	opt_init();
	cooldown = -1;
	set = ren = app = 0;
	while ((opt = getopt_long(c->argc, c->argv, "A:ac:r", long_opts))
			!= EOF) {
		switch (opt) {
		case 'A':
			if (strcmp(optarg, "on") == 0) {
				set = ON;
			} else if (strcmp(optarg, "off") == 0) {
				set = OFF;
			} else {
				_sprintf(out, MAX_MSG, "%s: -A setting must "
						"be on/off", c->argv[0]);
				return "";
			}
			break;
		case 'a':
			app = 1;
			break;
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
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'r':
			ren = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (ren) {
		if (app || cooldown != -1 || set)
			_sprintf(out, MAX_MSG, "%s: cannot use other flags "
					"with -r", c->argv[0]);
		else
			rename(out, m_customCmds, c);
	} else {
		edit(out, m_customCmds, c, cooldown, &m_cooldowns);
	}
	return "";
}

/* edit: edit a custom command */
static void edit(char *out, CustomCommandHandler *cch, struct command *c,
		time_t cooldown, TimerManager *tm)
{
	int cd, resp;
	char response[MAX_MSG];
	char buf[MAX_MSG];
	Json::Value *com;

	/* determine which parts are being changed */
	cd = cooldown != -1;
	resp = optind != c->argc - 1;

	argvcat(response, c->argc, c->argv, optind + 1, 1);

	if (app) {
		if ((com = cch->getcom(c->argv[optind]))->empty()) {
			_sprintf(out, MAX_MSG, "%s: not a command: $%s",
					c->argv[0], c->argv[optind]);
			return;
		}
		strcpy(buf, response);
		_sprintf(response, MAX_MSG, "%s %s",
				(*com)["response"].asCString(), buf);
	}

	if (!cch->editcom(c->argv[optind], response, cooldown)) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
		return;
	}
	if (!cd && !resp && !set) {
		_sprintf(out, MAX_MSG, "@%s, command $%s was unchanged",
				c->nick, c->argv[optind]);
		return;
	}

	/* build an output string detailing changes */
	_sprintf(out, MAX_MSG, "@%s, command $%s has been",
			c->nick, c->argv[optind]);
	if (set) {
		if (set == ON) {
			if (!cch->activate(c->argv[optind])) {
				_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
						cch->error().c_str());
				return;
			}
			strcat(out, " activated");
		} else {
			cch->deactivate(c->argv[optind]);
			strcat(out, " deactivated");
		}
	}
	if (resp || cd) {
		if (set)
			strcat(out, " and");
		strcat(out, " changed to ");
		out = strchr(out, '\0');
		if (resp) {
			_sprintf(out, MAX_MSG, "\"%s\"%s", response,
					cd ? ", with " : "");
			out = strchr(out, '\0');
		}
		if (cd) {
			/* reset cooldown in TimerManager */
			tm->remove(c->argv[optind]);
			tm->add(c->argv[optind], cooldown);
			_sprintf(out, MAX_MSG, "a %lds cooldown", cooldown);
		}
	}
	strcat(out, ".");
}

/* rename: rename a custom command */
static void rename(char *out, CustomCommandHandler *cch, struct command *c)
{
	if (optind != c->argc - 2)
		_USAGEMSG(out, _CMDNAME, RUSAGE);
	else if (!cch->rename(c->argv[optind], c->argv[optind + 1]))
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0],
				cch->error().c_str());
	else
		_sprintf(out, MAX_MSG, "@%s, command $%s has been renamed "
				"to $%s", c->nick, c->argv[optind],
				c->argv[optind + 1]);
}
