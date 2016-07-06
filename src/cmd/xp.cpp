#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("xp");
/* description of the command */
CMDDESCR("query experience information");
/* command usage synopsis */
CMDUSAGE("$xp [-i] NUM");

#define MAX_XP 0xBEBC200

static std::string xptolvl(int x);
static std::string lvltoxp(int x);

/* xp: query experience information */
std::string CommandHandler::xp(struct cmdinfo *c)
{
	bool inv;
	int x;
	std::string lvl;

	int opt;
	OptionParser op(c->fullCmd, "i");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "inverse", NO_ARG, 'i' },
		{ 0, 0, 0 }
	};

	inv = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'i':
			inv = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()
			|| (lvl = c->fullCmd.substr(op.optind())).find(' ')
			!= std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	try {
		if ((x = std::stoi(lvl)) < 0)
			return CMDNAME + ": number cannot be negative";
	} catch (std::invalid_argument) {
			return CMDNAME + ": invalid number -- '" + lvl + "'";
	}


	if (inv) {
		if (x > MAX_XP)
			return CMDNAME + ": xp cannot exceed 200m";
		return "[XP] " + utils::formatInteger(std::to_string(x))
			+ " xp: level " + xptolvl(x);
	}

	if (x < 1 || x > 126)
		return CMDNAME + ": level must be between 1-126";
	return "[XP] level " + std::to_string(x) + ": " + lvltoxp(x) + " xp";
}

/* xptolvl: calculate the level at x xp */
static std::string xptolvl(int x)
{
	int n;

	n = 1;
	x *= 4;
	x += 1;
	while (x >= 0) {
		x -= floor(n + 300 * pow(2, n / 7.0));
		++n;
	}

	return std::to_string(n - 1);
}

/* lvltoxp: return xp required for level x */
static std::string lvltoxp(int x)
{
	int n, res;

	res = 0;
	for (n = 1; n < x; ++n)
		res += floor(n + 300 * pow(2, n / 7.0));
	res = floor(0.25 * res);

	return utils::formatInteger(std::to_string(res));
}
