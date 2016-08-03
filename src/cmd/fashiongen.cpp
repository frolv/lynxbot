#include <string.h>
#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

#define F_SIZE 256

/* full name of the command */
CMDNAME("fashiongen");
/* description of the command */
CMDDESCR("generate an outfit");
/* command usage synopsis */
CMDUSAGE("$fashiongen");

static void gen_fashion(char *out, const Json::Value &items);

/* fashiongen: generate an outfit */
int CmdHandler::fashiongen(char *out, struct command *c)
{
	char fashion[F_SIZE];
	int opt, status;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	status = EXIT_SUCCESS;
	while ((opt = getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (optind != c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		status = EXIT_FAILURE;
	} else if (m_fashion.empty()) {
		_sprintf(out, MAX_MSG, "%s: could not read item database",
				c->argv[0]);
		status = EXIT_FAILURE;
	} else {
		gen_fashion(fashion, m_fashion);
		_sprintf(out, MAX_MSG, "[FASHIONGEN] %s",
				utils::upload(fashion).c_str());
	}
	return status;
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
