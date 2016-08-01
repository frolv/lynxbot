#include <string.h>
#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define F_SIZE 256

/* full name of the command */
_CMDNAME("fashiongen");
/* description of the command */
_CMDDESCR("generate an outfit");
/* command usage synopsis */
_CMDUSAGE("$fashiongen");

static void gen_fashion(char *out, const Json::Value &items);

/* fashiongen: generate an outfit */
std::string CommandHandler::fashiongen(char *out, struct command *c)
{
	char fashion[F_SIZE];
	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
	} else if (m_fashion.empty()) {
		_sprintf(out, MAX_MSG, "%s: could not read item database",
				c->argv[0]);
	} else {
		gen_fashion(fashion, m_fashion);
		_sprintf(out, MAX_MSG, "[FASHIONGEN] %s",
				utils::upload(fashion).c_str());
	}
	return "";
}

/* gen_fashion: generate a random outfit from items */
static void gen_fashion(char *out, const Json::Value &items)
{
	Json::Value::Members categories;
	srand(static_cast<uint32_t>(time(nullptr)));
	int ind;

	_sprintf(out, MAX_MSG, "Generated FashionScape\n"
			"======================\n\n");
	categories = items.getMemberNames();
	for (const std::string &cat : categories) {
		ind = rand() % items[cat].size();
		out = strchr(out, '\0');
		_sprintf(out, MAX_MSG, "%c%s:\t%s\n", toupper(cat[0]),
				cat.substr(1).c_str(),
				items[cat][ind].asCString());
	}
}
