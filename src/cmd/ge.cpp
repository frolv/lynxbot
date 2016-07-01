#include <algorithm>
#include <cstring>
#include <cpr/cpr.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("ge");
/* description of the command */
CMDDESCR("look up item prices");
/* command usage synopsis */
CMDUSAGE("$ge [-n AMT] ITEM");

static const std::string EXCHANGE_API =
	"https://api.rsbuddy.com/grandExchange?a=guidePrice&i=";

static bool getamt(char *num, int64_t *amt);
static int64_t extract_price(const std::string &resp);

/* ge: look up item prices */
std::string CommandHandler::ge(struct cmdinfo *c)
{
	if (!m_GEReader.active())
		return CMDNAME + ": GE reader is inactive";

	std::string output, name;
	int64_t amt, price;
	cpr::Response resp;
	Json::Value item;

	int opt;
	OptionParser op(c->fullCmd, "n:");
	static struct OptionParser::option long_opts[] = {
		{ "amount", REQ_ARG, 'n' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	amt = 1;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'n':
			if (!getamt(op.optarg(), &amt))
				return CMDNAME + ": invalid number -- '"
					+ std::string(op.optarg()) + "'";
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	name = c->fullCmd.substr(op.optind());
	std::replace(name.begin(), name.end(), '_', ' ');

	/* find item in database */
	if ((item = m_GEReader.getItem(name)).empty())
		return CMDNAME + ": item not found: " + name;

	/* read ge api and extract data */
	resp = cpr::Get(cpr::Url(EXCHANGE_API + item["id"].asString()),
		cpr::Header{{ "Connection", "close" }});
	if ((price = extract_price(resp.text)) == -1)
		return CMDNAME + ": could not extract price";

	output = "[GE] ";
	if (amt != 1)
		output += utils::formatInteger(std::to_string(amt)) + "x ";
	output += item["name"].asString() + ": ";
	output += utils::formatInteger(std::to_string(amt * price)) + " gp";

	return output;
}

/* getamt: read number from string num into amt */
static bool getamt(char *num, int64_t *amt)
{
	int n, last, mult;

	last = strlen(num) - 1;
	mult = 1;
	if (num[last] == 'k' || num[last] == 'm' || num[last] == 'b') {
		switch (num[last]) {
		case 'k':
			mult = 1000;
			break;
		case 'm':
			mult = 1000000;
			break;
		case 'b':
			mult = 1000000000;
			break;
		}
		num[last] = '\0';
	}

	n = 0;
	for (; *num; ++num) {
		if (*num < '0' || *num > '9')
			return false;
		n = 10 * n + (*num - '0');
	}
	*amt = n * mult;
	return true;
}

/* extract_price: return the price of the json data in resp */
static int64_t extract_price(const std::string &resp)
{
	Json::Reader reader;
	Json::Value item;
	if (reader.parse(resp, item))
		return item["overall"].asInt();
	else
		return -1;
}
