#include <json/json.h>
#include <string.h>
#include <time.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define MAX_PRINT 128

/* full name of the command */
CMDNAME("command");
/* description of the command */
CMDDESCR("show information about a custom command");
/* command usage synopsis */
CMDUSAGE("$cmdinfo [-aCcmu] CMD");

static int mod;
static int atime, crtime, creator, mtime, uses;

static void putinfo(char *out, Json::Value *cmd);

/* cmdinfo: show information about a custom command */
std::string CommandHandler::cmdinfo(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "atime", NO_ARG, 'a' },
		{ "creator", NO_ARG, 'C' },
		{ "ctime", NO_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "mtime", NO_ARG, 'm' },
		{ "uses", NO_ARG, 'u' },
		{ 0, 0, 0 }
	};

	opt_init();
	atime = crtime = creator = mtime = uses = mod = 0;
	while ((opt = getopt_long(c->argc, c->argv, "aCcmu", long_opts))
			!= EOF) {
		switch (opt) {
		case 'a':
			mod = atime = 1;
			break;
		case 'C':
			mod = creator = 1;
			break;
		case 'c':
			mod = crtime = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case 'm':
			mod = mtime = 1;
			break;
		case 'u':
			mod = uses = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}
	if (!mod)
		atime = crtime = creator = mtime = uses = 1;

	if (optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return "";
	}

	switch (source(c->argv[optind])) {
	case DEFAULT:
		_sprintf(out, MAX_MSG, "[CMDINFO] %s is a default command",
				c->argv[optind]);
		break;
	case CUSTOM:
		putinfo(out, m_customCmds->getcom(c->argv[optind]));
		break;
	default:
		_sprintf(out, MAX_MSG, "%s: command not found: %s",
				c->argv[0], c->argv[optind]);
		break;
	}
	return "";
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

#ifdef __linux__
	atm = *localtime(&at);
	ctm = *localtime(&ct);
	mtm = *localtime(&mt);
#endif
#ifdef _WIN32
	localtime_s(&atm, &at);
	localtime_s(&ctm, &ct);
	localtime_s(&mtm, &mt);
#endif

	_sprintf(out, MAX_MSG, "[CMDINFO] %s: ", (*cmd)["cmd"].asCString());
	end = out + strlen(out);
	if (crtime || creator) {
		strcat(end, " Created");
		end = strchr(end, '\0');
		if (creator) {
			_sprintf(end, MAX_PRINT, " by %s",
					(*cmd)["creator"].asCString());
			end = strchr(end, '\0');
		}
		if (crtime) {
			strcat(end, " at ");
			end = strchr(end, '\0');
			strftime(end, MAX_PRINT, "%R %Z %d/%m/%Y", &ctm);
			end = strchr(end, '\0');
			_sprintf(end, MAX_PRINT, " (%s ago)",
					utils::conv_time(time(nullptr) - ct)
					.c_str());
		}
		strcat(end, ".");
		end = strchr(end, '\0');
	}
	if (mtime) {
		if ((!mod && ct != mt) || mod) {
			strcat(end, " Last modified at ");
			end = strchr(end, '\0');
			strftime(end, MAX_PRINT, "%R %Z %d/%m/%Y", &mtm);
			end = strchr(end, '\0');
			_sprintf(end, MAX_PRINT, " (%s ago).",
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
			_sprintf(end, MAX_PRINT, " (%s ago).",
					utils::conv_time(time(nullptr) - at)
					.c_str());
			end = strchr(end, '\0');
		} else if (mod) {
			strcat(end, " Never used.");
			end = strchr(end, '\0');
		}
	}
	if (uses)
		_sprintf(end, MAX_PRINT, " Used %d time%s.",
				(*cmd)["uses"].asInt(),
				(*cmd)["uses"].asInt() == 1 ? "" : "s");
}
