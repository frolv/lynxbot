#include <json/json.h>
#include <regex>
#include <stdlib.h>
#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

#define ALL	0
#define DEFAULT	1
#define CUSTOM	2

/* full name of the command */
CMDNAME("cgrep");
/* description of the command */
CMDDESCR("find commands matching a pattern");
/* command usage synopsis */
CMDUSAGE("$cgrep [-ai] [-c|-d] PATTERN");

/* only show active commands */
static int act;

/* ignore case */
static int ign;

/* which commands to search through */
static int type;

static int findcmds(char *out, const CmdHandler::cmdmap *cmap,
		const Json::Value *customs, const char *pat);
static void format(char *out, const char **def, const char **cus);
static void format_ul(char *out, const char **def, const char **cus);
static int cmp(const void *a, const void *b);

/* cgrep: find commands matching a pattern */
int CmdHandler::cgrep(char *out, struct command *c)
{
	int opt;
	static struct l_option long_opts[] = {
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
	while ((opt = l_getopt_long(c->argc, c->argv, "acdi",
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
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'i':
			ign = 1;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	return findcmds(out, &default_cmds, custom_cmds->commands(),
				c->argv[l_optind]);
}

/* findcmds: find any bot commands that match pat */
static int findcmds(char *out, const CmdHandler::cmdmap *cmap,
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
		snprintf(out, MAX_MSG, "%s: invalid regular expression",
				CMDNAME);
		return EXIT_FAILURE;
	}

	def = (const char **)malloc((cmap->size() + 1) * sizeof(*def));
	cus = (const char **)malloc(((*customs)["commands"].size() + 1)
			* sizeof(*cus));

	nmatch = cmdlen = i = j = 0;
	/* search default commands */
	if (type == ALL || type == DEFAULT) {
		for (const auto &p : *cmap) {
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
		snprintf(out, MAX_MSG, "[CGREP] no matches found for '%s'", pat);
		return EXIT_FAILURE;
	}

	qsort(def, i, sizeof(*def), cmp);
	qsort(cus, j, sizeof(*cus), cmp);

	/* check if message is too long for twitch and upload */
	if (cmdlen > 260) {
		ul = (char *)malloc((cmdlen * 4 + 128) * sizeof(*ul));
		snprintf(ul, MAX_MSG, "%d command%s found for '%s':",
				nmatch, nmatch == 1 ? "" : "s", pat);
		format_ul(ul, def, cus);
		snprintf(out, MAX_MSG, "[CGREP] %s", utils::upload(ul).c_str());
		free(ul);
	} else {
		snprintf(out, MAX_MSG, "[CGREP] %d command%s found for '%s':",
				nmatch, nmatch == 1 ? "" : "s", pat);
		format(out, def, cus);
	}
	free(def);
	free(cus);
	return EXIT_SUCCESS;
}

/* format: return a formatted string of all found commands */
static void format(char *out, const char **def, const char **cus)
{
	char *end;

	if (*def)
		strcat(out, " (DEFAULT)");
	end = out + strlen(out);
	for (; *def; ++def) {
		snprintf(end, 48, " \"%s\",", *def);
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
		snprintf(end, 48, " \"%s\",", *cus);
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
		snprintf(end, 48, " \"%s\"\n", *def);
		end = strchr(end, '\0');
	}
	if (*cus) {
		if (*def)
			strcat(end, "\n================");
		strcat(end, "\n\nCUSTOM:\n\n");
		end = strchr(end, '\0');
	}
	for (; *cus; ++cus) {
		snprintf(end, 48, " \"%s\"\n", *cus);
		end = strchr(end, '\0');
	}
}

static int cmp(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}
