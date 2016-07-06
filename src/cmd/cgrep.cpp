#include <json/json.h>
#include <regex>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

#define ALL	0
#define DEFAULT	1
#define CUSTOM	2

/* full name of the command */
CMDNAME("cgrep");
/* description of the command */
CMDDESCR("find commands matching a pattern");
/* command usage synopsis */
CMDUSAGE("$cgrep [-cdi] PATTERN");

static std::string findcmds(const CommandHandler::commandMap *cmdmap,
		const Json::Value *customs, const std::string &pat,
		int type, bool ign);
static std::string format(std::vector<std::string> def,
		std::vector<std::string> cus);

/* cgrep: find commands matching a pattern */
std::string CommandHandler::cgrep(struct cmdinfo *c)
{
	std::string pattern;
	bool ign;

	int opt, type;
	OptionParser op(c->fullCmd, "cdi");
	static struct OptionParser::option long_opts[] = {
		{ "custom", NO_ARG, 'c' },
		{ "default", NO_ARG, 'd' },
		{ "help", NO_ARG, 'h' },
		{ "ignore-case", NO_ARG, 'i' },
		{ 0, 0, 0 }
	};

	ign = false;
	type = ALL;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			type = CUSTOM;
			break;
		case 'd':
			type = DEFAULT;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'i':
			ign = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()
			|| (pattern = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	return findcmds(&m_defaultCmds, m_customCmds->commands(),
			pattern, type, ign);
}

/* findcmds: find any bot commands that match pat */
static std::string findcmds(const CommandHandler::commandMap *cmdmap,
		const Json::Value *customs, const std::string &pat,
		int type, bool ign)
{
	std::string out;
	std::vector<std::string> def, cus;
	int nmatch;
	std::regex reg;
	std::smatch match;

	try {
		if (ign)
			reg = std::regex(pat, std::regex::icase);
		else
			reg = std::regex(pat);
		nmatch = 0;
	} catch (std::regex_error) {
		return CMDNAME + ": invalid regular expression";
	}

	/* search default commands */
	if (type == ALL || type == DEFAULT) {
		for (const auto &p : *cmdmap) {
			if (std::regex_search(p.first.begin(), p.first.end(),
						match, reg)) {
				++nmatch;
				def.emplace_back(p.first);
			}
		}
	}
	/* search custom commands */
	if (type == ALL || type == CUSTOM) {
		for (const auto &val : (*customs)["commands"]) {
			const std::string cmd = val["cmd"].asString();
			if (std::regex_search(cmd.begin(), cmd.end(),
						match, reg)) {
				++nmatch;
				cus.emplace_back(cmd);
			}
		}
	}

	if (!nmatch)
		return "[CGREP] no matches found for '" + pat + "'";

	out = "[CGREP] ";
	out += std::to_string(nmatch) + " command" + ((nmatch == 1) ? "" : "s")
		+ " found for '" + pat + "':";
	return out + format(def, cus);
}

/* format: return a formatted string of all found commands */
static std::string format(std::vector<std::string> def,
		std::vector<std::string> cus)
{
	std::string out;
	size_t i;

	std::sort(def.begin(), def.end());
	std::sort(cus.begin(), cus.end());

	if (!def.empty())
		out += " (DEFAULT)";
	for (i = 0; i < def.size(); ++i) {
		out += " \"" + def[i] + "\"";
		if (i != def.size() - 1)
			out += ",";
	}
	if (!cus.empty()) {
		if (!def.empty())
			out += " |";
		out += " (CUSTOM)";
	}
	for (i = 0; i < cus.size(); ++i) {
		out += " \"" + cus[i] + "\"";
		if (i != cus.size() - 1)
			out += ",";
	}
	return out;
}
