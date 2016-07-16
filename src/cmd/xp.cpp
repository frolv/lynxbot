#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("xp");
/* description of the command */
CMDDESCR("query experience information");
/* command usage synopsis */
CMDUSAGE("$xp [-i] [-r] NUM");

#define MAX_XP 0xBEBC200

static int xptolvl(int x);
static int lvltoxp(int x);
static std::string xprange(const std::string &args);

/* xp: query experience information */
std::string CommandHandler::xp(struct cmdinfo *c)
{
	bool inv, range;
	int x;
	std::string lvl;

	int opt;
	OptionParser op(c->fullCmd, "ir");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "inverse", NO_ARG, 'i' },
		{ "range", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	inv = range = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'i':
			inv = true;
			break;
		case 'r':
			range = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (range) {
		if (inv)
			return CMDNAME + ": cannot combine -r with -i";
		return xprange(c->fullCmd.substr(op.optind()));
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
	} catch (std::out_of_range) {
		return CMDNAME + ": number too large";
	}


	if (inv) {
		if (x > MAX_XP)
			return CMDNAME + ": xp cannot exceed 200m";
		return "[XP] " + utils::formatInteger(std::to_string(x))
			+ " xp: level " + std::to_string(xptolvl(x));
	}

	if (x < 1 || x > 126)
		return CMDNAME + ": level must be between 1-126";
	return "[XP] level " + std::to_string(x) + ": "
		+ utils::formatInteger(std::to_string(lvltoxp(x))) + " xp";
}

/* xptolvl: calculate the level at x xp */
static int xptolvl(int x)
{
	int n;

	n = 1;
	x *= 4;
	x += 1;
	while (x >= 0) {
		x -= floor(n + 300 * pow(2, n / 7.0));
		++n;
	}

	return n - 1;
}

/* lvltoxp: return xp required for level x */
static int lvltoxp(int x)
{
	int n, res;

	res = 0;
	for (n = 1; n < x; ++n)
		res += floor(n + 300 * pow(2, n / 7.0));
	res = floor(0.25 * res);

	return res;
}

/* xprange: calcuate the amount of xp between the levels in args */
static std::string xprange(const std::string &args)
{
	std::string a, b;
	size_t i;
	int x, y;

	i = 0;
	/* read the first level */
	while (i < args.length() && !isspace(args[i]) && args[i] != '-')
		a += args[i++];
	if (i == args.length())
		return CMDNAME + ": must provide two levels";

	++i;
	/* read the second level */
	while (i < args.length() && !isspace(args[i]))
		b += args[i++];
	if (i != args.length())
		return CMDNAME + ": must provide two levels";

	try {
		if ((x = std::stoi(a)) < 1 || x > 126)
			return CMDNAME + ": level must be between 1-126";
	} catch (std::invalid_argument) {
		return CMDNAME + ": invalid number -- '" + a + "'";
	} catch (std::out_of_range) {
		return CMDNAME + ": number too large";
	}
	try {
		if ((y = std::stoi(b)) < 1 || y > 126)
			return CMDNAME + ": level must be between 1-126";
	} catch (std::invalid_argument) {
		return CMDNAME + ": invalid number -- '" + b + "'";
	} catch (std::out_of_range) {
		return CMDNAME + ": number too large";
	}

	if (x > y)
		return CMDNAME + ": invalid range";
	x = lvltoxp(x);
	y = lvltoxp(y);

	return "[XP] level " + a + "-" + b + ": "
		+ utils::formatInteger(std::to_string(y - x)) + " xp";
}
