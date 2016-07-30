#include <json/json.h>
#include <regex>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define ALL	0
#define DEFAULT	1
#define CUSTOM	2

/* full name of the command */
_CMDNAME("cgrep");
/* description of the command */
_CMDDESCR("find commands matching a pattern");
/* command usage synopsis */
_CMDUSAGE("$cgrep [-ai] [-c|-d] PATTERN");

/* only show active commands */
static int act;

/* ignore case */
static int ign;

/* which commands to search through */
static int type;

static void findcmds(char *out, const CommandHandler::commandMap *cmdmap,
		const Json::Value *customs, const char *pat);
static void format(char *out, const char **def, const char **cus);
static void format_ul(char *out, const char **def, const char **cus);
static int cmp(const void *a, const void *b);

/* cgrep: find commands matching a pattern */
std::string CommandHandler::cgrep(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "active", NO_ARG, 'a' },
		{ "custom", NO_ARG, 'c' },
		{ "default", NO_ARG, 'd' },
		{ "help", NO_ARG, 'h' },
		{ "ignore-case", NO_ARG, 'i' },
		{ 0, 0, 0 }
	};

	opt_init();
	act = ign = 0;
	type = ALL;
	while ((opt = getopt_long(c->argc, c->argv, "acdi",
					long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			act = 1;
			break;
		case 'c':
			type = CUSTOM;
			break;
		case 'd':
			type = DEFAULT;
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'i':
			ign = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc - 1)
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
	else
		findcmds(out, &m_defaultCmds, m_customCmds->commands(),
				c->argv[optind]);
	return "";
}

/* findcmds: find any bot commands that match pat */
static void findcmds(char *out, const CommandHandler::commandMap *cmdmap,
		const Json::Value *customs, const char *pat)
{
	const char **def, **cus;
	char *ul;
	size_t i, j;
	int nmatch, cmdlen;
	std::regex reg;
	std::smatch match;

	try {
		if (ign)
			reg = std::regex(pat, std::regex::icase);
		else
			reg = std::regex(pat);
	} catch (std::regex_error) {
		_sprintf(out, MAX_MSG, "%s: invalid regular expression",
				_CMDNAME);
		return;
	}

	def = (const char **)malloc((cmdmap->size() + 1) * sizeof(*def));
	cus = (const char **)malloc(((*customs)["commands"].size() + 1) * sizeof(*cus));

	nmatch = cmdlen = i = j = 0;
	/* search default commands */
	if (type == ALL || type == DEFAULT) {
		for (const auto &p : *cmdmap) {
			if (std::regex_search(p.first.begin(), p.first.end(),
						match, reg)) {
				++nmatch;
				def[i++] = p.first.c_str();
				cmdlen += p.first.length();
			}
		}
	}
	def[i] = NULL;
	/* search custom commands */
	if (type == ALL || type == CUSTOM) {
		for (const auto &val : (*customs)["commands"]) {
			const std::string cmd = val["cmd"].asString();
			/* skip inactive commands with -a flag */
			if (act && !val["active"].asBool())
				continue;
			if (std::regex_search(cmd.begin(), cmd.end(),
						match, reg)) {
				++nmatch;
				cus[j++] = val["cmd"].asCString();
				cmdlen += cmd.length();
			}
		}
	}
	cus[j] = NULL;

	if (!nmatch) {
		_sprintf(out, MAX_MSG, "[CGREP] no matches found for '%s'", pat);
		return;
	}

	qsort(def, i, sizeof(*def), cmp);
	qsort(cus, j, sizeof(*cus), cmp);

	/* check if message is too long for twitch and upload */
	if (cmdlen > 260) {
		ul = (char *)malloc((cmdlen * 4 + 128) * sizeof(*ul));
		_sprintf(ul, MAX_MSG, "%d command%s found for '%s':",
				nmatch, nmatch == 1 ? "" : "s", pat);
		format_ul(ul, def, cus);
		_sprintf(out, MAX_MSG, "[CGREP] %s", utils::upload(ul).c_str());
		free(ul);
	} else {
		_sprintf(out, MAX_MSG, "[CGREP] %d command%s found for '%s':",
				nmatch, nmatch == 1 ? "" : "s", pat);
		format(out, def, cus);
	}
	free(def);
	free(cus);
}

/* format: return a formatted string of all found commands */
static void format(char *out, const char **def, const char **cus)
{
	char *end;

	if (*def)
		strcat(out, " (DEFAULT)");
	end = out + strlen(out);
	for (; *def; ++def) {
		_sprintf(end, 48, " \"%s\",", *def);
		end = strchr(end, '\0');
	}
	if (end[-1] == ',')
		*--end = '\0';
	if (*cus) {
		if (*def)
			strcat(end, " |");
		strcat(end, " (CUSTOM)");
		end = strchr(end, '\0');
	}
	for (; *cus; ++cus) {
		_sprintf(end, 48, " \"%s\",", *cus);
		end = strchr(end, '\0');
	}
	if (end[-1] == ',')
		*--end = '\0';
}

/* format_ul: format commands for upload to ptpb */
static void format_ul(char *out, const char **def, const char **cus)
{
	char *end;

	if (*def)
		strcat(out, "\n\nDEFAULT:\n\n");
	end = out + strlen(out);
	for (; *def; ++def) {
		_sprintf(end, 48, " \"%s\"\n", *def);
		end = strchr(end, '\0');
	}
	if (*cus) {
		if (*def)
			strcat(end, "\n================");
		strcat(end, "\n\nCUSTOM:\n\n");
		end = strchr(end, '\0');
	}
	for (; *cus; ++cus) {
		_sprintf(end, 48, " \"%s\"\n", *cus);
		end = strchr(end, '\0');
	}
}

static int cmp(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}
