#include <string.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"
#include "../strfmt.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("xp");
/* description of the command */
CMDDESCR("query experience information");
/* command usage synopsis */
CMDUSAGE("$xp [-i|-r] NUM");
/* -r flag usage */
static const char *RUSAGE = "$xp -r START STOP";

#define MAX_XP 0xBEBC200

static int xptolvl(int x);
static int lvltoxp(int x);
static int xprange(char *out, struct command *c);

/* xp: query experience information */
int CommandHandler::xp(char *out, struct command *c)
{
	int inv, range, status;
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
	status = EXIT_SUCCESS;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "ir", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 'i':
			inv = 1;
			break;
		case 'r':
			range = 1;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (range) {
		if (inv) {
			_sprintf(out, MAX_MSG, "%s: cannot use -i with -r",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			status = xprange(out, c);
		}
		return status;
	}

	if (optind != c->argc - 1) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (!parsenum(c->argv[optind], &x)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s",
				c->argv[0], c->argv[optind]);
		return EXIT_FAILURE;
	}
	if (x < 0) {
		_sprintf(out, MAX_MSG, "%s: number cannot be "
				"negative", c->argv[0]);
		return EXIT_FAILURE;
	}

	if (inv) {
		if (x > MAX_XP) {
			_sprintf(out, MAX_MSG, "%s: xp cannot exceed 200m",
					c->argv[0]);
			status = EXIT_FAILURE;
		} else {
			fmtnum(num, RSN_BUF, c->argv[optind]);
			_sprintf(out, MAX_MSG, "[XP] %s xp: level %d",
					num, xptolvl(x));
		}
		return status;
	}

	if (x < 1 || x > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
		status = EXIT_FAILURE;
	} else {
		_sprintf(out, MAX_MSG, "%d", lvltoxp(x));
		fmtnum(num, RSN_BUF, out);
		_sprintf(out, MAX_MSG, "[XP] level %ld: %s xp", x, num);
	}
	return status;
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
static int xprange(char *out, struct command *c)
{
	int64_t x, y;
	char *a, *b;
	char num[RSN_BUF];

	if (optind == c->argc || optind < c->argc - 2) {
		USAGEMSG(out, CMDNAME, RUSAGE);
		return EXIT_FAILURE;
	}

	a = c->argv[optind];
	if (optind == c->argc - 1) {
		if (!(b = strchr(a, '-')) || !b[1]) {
			_sprintf(out, MAX_MSG, "%s: must provide two levels",
					c->argv[0]);
			return EXIT_FAILURE;
		}
		*b++ = '\0';
	} else {
		b = c->argv[optind + 1];
	}

	if (!parsenum(a, &x)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s", c->argv[0], a);
		return EXIT_FAILURE;
	}
	if (x < 1 || x > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
		return EXIT_FAILURE;
	}
	if (!parsenum(b, &y)) {
		_sprintf(out, MAX_MSG, "%s: invalid number: %s", c->argv[0], b);
		return EXIT_FAILURE;
	}
	if (y < 1 || y > 126) {
		_sprintf(out, MAX_MSG, "%s: level must be between 1-126",
				c->argv[0]);
		return EXIT_FAILURE;
	}

	if (x > y) {
		_sprintf(out, MAX_MSG, "%s: invalid range", c->argv[0]);
		return EXIT_FAILURE;
	}

	x = lvltoxp(x);
	y = lvltoxp(y);

	_sprintf(out, MAX_MSG, "%ld", y - x);
	fmtnum(num, RSN_BUF, out);
	_sprintf(out, MAX_MSG, "[XP] level %s-%s: %s xp", a, b, num);
	return EXIT_SUCCESS;
}
