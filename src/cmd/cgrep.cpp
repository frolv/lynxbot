#include <json/json.h>
#include <regex>
#include <utils.h>
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
static std::string format(const std::vector<std::string> &def,
		const std::vector<std::string> &cus);
static std::string format_ul(const std::vector<std::string> &def,
		const std::vector<std::string> &cus);

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
	int nmatch, cmdlen;
	std::regex reg;
	std::smatch match;

	try {
		if (ign)
			reg = std::regex(pat, std::regex::icase);
		else
			reg = std::regex(pat);
	} catch (std::regex_error) {
		return CMDNAME + ": invalid regular expression";
	}

	nmatch = cmdlen = 0;
	/* search default commands */
	if (type == ALL || type == DEFAULT) {
		for (const auto &p : *cmdmap) {
			if (std::regex_search(p.first.begin(), p.first.end(),
						match, reg)) {
				++nmatch;
				def.emplace_back(p.first);
				cmdlen += p.first.length();
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
				cmdlen += cmd.length();
			}
		}
	}

	if (!nmatch)
		return "[CGREP] no matches found for '" + pat + "'";

	std::sort(def.begin(), def.end());
	std::sort(cus.begin(), cus.end());

	out += std::to_string(nmatch) + " command" + (nmatch == 1 ? "" : "s")
		+ " found for '" + pat + "':";

	/* check if message is too long for twitch and upload */
	if (cmdlen > 260) {
		out += "\n" + format_ul(def, cus);
		return "[CGREP] " + utils::upload(out);
	} else {
		return "[CGREP] " + out + format(def, cus);
	}
}

/* format: return a formatted string of all found commands */
static std::string format(const std::vector<std::string> &def,
		const std::vector<std::string> &cus)
{
	std::string out;
	size_t i;

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

/* format_ul: format commands for upload to ptpb */
static std::string format_ul(const std::vector<std::string> &def,
		const std::vector<std::string> &cus)
{
	std::string out;
	size_t i;

	if (!def.empty())
		out += "\nDEFAULT:\n\n";
	for (i = 0; i < def.size(); ++i)
		out += "\"" + def[i] + "\"\n";
	if (!cus.empty()) {
		if (!def.empty())
			out += "\n================\n";
		out += "\nCUSTOM:\n\n";
	}
	for (i = 0; i < cus.size(); ++i)
		out += "\"" + cus[i] + "\"\n";
	return out;
}
