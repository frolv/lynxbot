#include <algorithm>
#include <string.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../ExpressionParser.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
_CMDNAME("calc");
/* description of the command */
_CMDDESCR("perform basic calculations");
/* command usage synopsis */
_CMDUSAGE("$calc EXPR");

/* calc: perform basic calculations */
std::string CommandHandler::calc(char *out, struct command *c)
{
	char expr[MAX_MSG];
	char *s;

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

	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	expr[0] = '\0';
	s = expr;
	for (; optind < c->argc; ++optind)
		strcat(expr, c->argv[optind]);
	/* remove whitespace */
	while ((s = strchr(s, ' ')))
		shift_l(s);

	try {
		ExpressionParser exp(expr);
		exp.tokenizeExpr();
		_sprintf(out, MAX_MSG, "[CALC] %f", exp.eval());
	} catch (std::runtime_error &e) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], e.what());
		return "";
	}
	if (strstr(out, "inf") || strstr(out, "-nan(ind)"))
		_sprintf(out, MAX_MSG, "%s: division by 0", c->argv[0]);

	/* remove trailing zeros */
	if (strchr(out, '.')) {
		s = out + strlen(out) - 1;
		while (*s == '0')
			*s-- = '\0';
		if (*s == '.')
			*s-- = '\0';
	}

	return "";
}
