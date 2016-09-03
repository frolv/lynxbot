#include <json/json.h>
#include <string.h>
#include <time.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

#define MAX_PRINT 128

/* full name of the command */
CMDNAME("cmdinfo");
/* description of the command */
CMDDESCR("show information about a custom command");
/* command usage synopsis */
CMDUSAGE("$cmdinfo [-aCcmu] CMD");

static int modified;
static int atime, crtime, creator, mtime, uses;

static void putinfo(char *out, Json::Value *cmd);

/* cmdinfo: show information about a custom command */
int CmdHandler::cmdinfo(char *out, struct command *c)
{
	int opt, status;
	static struct l_option long_opts[] = {
		{ "atime", NO_ARG, 'a' },
		{ "creator", NO_ARG, 'C' },
		{ "ctime", NO_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "mtime", NO_ARG, 'm' },
		{ "uses", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	opt_init();
	status = EXIT_SUCCESS;
	atime = crtime = creator = mtime = uses = modified = 0;
	while ((opt = l_getopt_long(c->argc, c->argv, "aCcmu", long_opts))
			!= EOF) {
		switch (opt) {
		case 'a':
			modified = atime = 1;
			break;
		case 'C':
			modified = creator = 1;
			break;
		case 'c':
			modified = crtime = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'm':
			modified = mtime = 1;
			break;
		case 'u':
			modified = uses = 1;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}
	if (!modified)
		atime = crtime = creator = mtime = uses = 1;

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	switch (source(c->argv[l_optind])) {
	case DEFAULT:
		snprintf(out, MAX_MSG, "[CMDINFO] %s is a default command",
				c->argv[l_optind]);
		break;
	case CUSTOM:
		putinfo(out, m_customCmds->getcom(c->argv[l_optind]));
		break;
	default:
		snprintf(out, MAX_MSG, "%s: command not found: %s",
				c->argv[0], c->argv[l_optind]);
		status = EXIT_FAILURE;
		break;
	}
	return status;
}

/* info: return requested information about cmd */
static void putinfo(char *out, Json::Value *cmd)
{
	char *end;
	time_t at, ct, mt;
	struct tm atm, ctm, mtm;

	if (cmd->isMember("atime"))
		at = (time_t)(*cmd)["atime"].asInt64();
	else
		at = 0;
	ct = (time_t)(*cmd)["ctime"].asInt64();
	mt = (time_t)(*cmd)["mtime"].asInt64();

	atm = *localtime(&at);
	ctm = *localtime(&ct);
	mtm = *localtime(&mt);
	printf("%d %d %d\n", ctm.tm_hour, ctm.tm_min, ctm.tm_sec);

	snprintf(out, MAX_MSG, "[CMDINFO] %s: ", (*cmd)["cmd"].asCString());
	end = out + strlen(out);
	if (crtime || creator) {
		strcat(end, " Created");
		end = strchr(end, '\0');
		if (creator) {
			snprintf(end, MAX_PRINT, " by %s",
					(*cmd)["creator"].asCString());
			end = strchr(end, '\0');
		}
		if (crtime) {
			strcat(end, " at ");
			end = strchr(end, '\0');
			/* strftime(end, MAX_PRINT, "%R %Z %d/%m/%Y", &ctm); */
			if (!strftime(end, MAX_PRINT, "%d", &ctm))
				perror("strftime");
			end = strchr(end, '\0');
			snprintf(end, MAX_PRINT, " (%s ago)",
					utils::conv_time(time(nullptr) - ct)
					.c_str());
		}
		strcat(end, ".");
		end = strchr(end, '\0');
	}
	if (mtime) {
		if ((!modified && ct != mt) || modified) {
			strcat(end, " Last modified at ");
			end = strchr(end, '\0');
			strftime(end, MAX_PRINT, "%R %Z %d/%m/%Y", &mtm);
			end = strchr(end, '\0');
			snprintf(end, MAX_PRINT, " (%s ago).",
					utils::conv_time(time(nullptr) - mt)
					.c_str());
			end = strchr(end, '\0');
		}
	}
	if (atime) {
		if (at) {
			strcat(end, " Last used at ");
			end = strchr(end, '\0');
			strftime(end, MAX_PRINT, "%R %Z %d/%m/%Y", &atm);
			end = strchr(end, '\0');
			snprintf(end, MAX_PRINT, " (%s ago).",
					utils::conv_time(time(nullptr) - at)
					.c_str());
			end = strchr(end, '\0');
		} else if (modified) {
			strcat(end, " Never used.");
			end = strchr(end, '\0');
		}
	}
	if (uses)
		snprintf(end, MAX_PRINT, " Used %d time%s.",
				(*cmd)["uses"].asInt(),
				(*cmd)["uses"].asInt() == 1 ? "" : "s");
}
