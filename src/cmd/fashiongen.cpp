#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("fashiongen");
/* description of the command */
CMDDESCR("generate an outfit");
/* command usage synopsis */
CMDUSAGE("$fashiongen");

static std::string gen_fashion(const Json::Value &items);

/* fashiongen: generate an outfit */
std::string CommandHandler::fashiongen(char *out, struct command *c)
{
	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (m_fashion.empty())
		return CMDNAME + ": could not read item database";

	return "[FASHIONGEN] " + utils::upload(gen_fashion(m_fashion));
}

/* gen_fashion: generate a random outfit from items */
static std::string gen_fashion(const Json::Value &items)
{
	std::string out;
	Json::Value::Members categories;
	srand(static_cast<uint32_t>(time(nullptr)));
	int ind;

	out = "Generated FashionScape\n";
	out += "======================\n\n";

	categories = items.getMemberNames();
	for (const std::string &cat : categories) {
		ind = rand() % items[cat].size();
		out += (char)toupper(cat[0]) + cat.substr(1) + ":\t";
		out += items[cat][ind].asString();
		out += "\n";
	}
	return out;
}
