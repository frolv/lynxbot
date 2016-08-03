#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../strfmt.h"
#include "../stringparse.h"

/* full name of the command */
_CMDNAME("xp");
/* description of the command */
_CMDDESCR("query experience information");
/* command usage synopsis */
_CMDUSAGE("$xp [-i|-r] NUM");
/* -r flag usage */
static const char *RUSAGE = "$xp -r START STOP";

#define MAX_XP 0xBEBC200

static int xptolvl(int x);
static int lvltoxp(int x);
static void xprange(char *out, struct command *c);

/* xp: query experience information */
std::string CommandHandler::xp(char *out, struct command *c)
{
	int inv, range;
	int64_t x;
	char num[RSN_BUF];

	int opt;
	static struct option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ "inverse", NO_ARG, 'i' },
		{ "range", NO_ARG, 'r' },
		{ 0, 0, 0 }
	};

	inv = range = 0;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "ir", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'i':
			inv = 1;
			break;
		case 'r':
			range = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (range) {
		if (inv)
			_sprintf(out, MAX_MSG, "%s: cannot use -i with -r",
					c->argv[0]);
		else
			xprange(out, c);
		return "";
	}

	if (optind != c->argc - 1) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	if (!parsenum(c->argv[optind], &x)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s",
				c->argv[0], c->argv[optind]);
		return "";
	}
	if (x < 0) {
		_sprintf(out, MAX_MSG, "%s: number cannot be "
				"negative", c->argv[0]);
		return "";
	}

	if (inv) {
		if (x > MAX_XP) {
			_sprintf(out, MAX_MSG, "%s: xp cannot exceed 200m",
					c->argv[0]);
		} else {
			fmtnum(num, RSN_BUF, c->argv[optind]);
			_sprintf(out, MAX_MSG, "[XP] %s xp: level %d",
					num, xptolvl(x));
		}
		return "";
	}

	if (x < 1 || x > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
	} else {
		_sprintf(out, MAX_MSG, "%d", lvltoxp(x));
		fmtnum(num, RSN_BUF, out);
		_sprintf(out, MAX_MSG, "[XP] level %ld: %s xp", x, num);
	}
	return "";
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
static void xprange(char *out, struct command *c)
{
	int64_t x, y;
	char *a, *b;
	char num[RSN_BUF];

	if (optind == c->argc || optind < c->argc - 2) {
		_USAGEMSG(out, _CMDNAME, RUSAGE);
		return;
	}

	a = c->argv[optind];
	if (optind == c->argc - 1) {
		if (!(b = strchr(a, '-')) || !b[1]) {
			_sprintf(out, MAX_MSG, "%s: must provide two levels",
					c->argv[0]);
			return;
		}
		*b++ = '\0';
	} else {
		b = c->argv[optind + 1];
	}

	if (!parsenum(a, &x)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s", c->argv[0], a);
		return;
	}
	if (x < 1 || x > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
		return;
	}
	if (!parsenum(b, &y)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s", c->argv[0], b);
		return;
	}
	if (y < 1 || y > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
		return;
	}

	if (x > y) {
		_sprintf(out, MAX_MSG, "%s: invalid range", c->argv[0]);
		return;
	}

	x = lvltoxp(x);
	y = lvltoxp(y);

	_sprintf(out, MAX_MSG, "%ld", y - x);
	fmtnum(num, RSN_BUF, out);
	_sprintf(out, MAX_MSG, "[XP] level %s-%s: %s xp", a, b, num);
}
