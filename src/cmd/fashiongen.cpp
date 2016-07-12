#include <cpr/cpr.h>
#include <tw/oauth.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("fashiongen");
/* description of the command */
CMDDESCR("generate an outfit");
/* command usage synopsis */
CMDUSAGE("$fashiongen");

static const std::string PB = "https://ptpb.pw/";

static std::string gen_fashion(const Json::Value &items);
static std::string upload(const std::string &s);

/* fashiongen: generate an outfit */
std::string CommandHandler::fashiongen(struct cmdinfo *c)
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

	return "[FASHIONGEN] " + upload(gen_fashion(m_fashion));
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
		out += tw::pencode(items[cat][ind].asString());
		out += "\n";
	}
	return out;
}

/* upload: upload s to ptpb.pw */
static std::string upload(const std::string &s)
{
	cpr::Response resp;
	size_t i;
	std::string url;

	resp = cpr::Post(cpr::Url(PB), cpr::Body("c=" + s));

	if ((i = resp.text.find("url:")) == std::string::npos)
		return CMDNAME + ": failed to upload";

	url = resp.text.substr(i + 5);
	if ((i = url.find('\n')) != std::string::npos)
		url = url.substr(0, i);

	return url;
}
