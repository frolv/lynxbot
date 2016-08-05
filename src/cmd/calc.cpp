#include <algorithm>
#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../ExpressionParser.h"
#include "../option.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("calc");
/* description of the command */
CMDDESCR("perform basic calculations");
/* command usage synopsis */
CMDUSAGE("$calc EXPR");

/* calc: perform basic calculations */
int CmdHandler::calc(char *out, struct command *c)
{
	char expr[MAX_MSG];
	char *s;

	int opt;
	static struct l_option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "", long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			_sprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	argvcat(expr, c->argc, c->argv, l_optind, 0);
	s = expr;
	while ((s = strchr(s, ' ')))
		shift_l(s);

	try {
		ExpressionParser exp(expr);
		exp.tokenizeExpr();
		_sprintf(out, MAX_MSG, "[CALC] %f", exp.eval());
	} catch (std::runtime_error &e) {
		_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], e.what());
		return EXIT_FAILURE;
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

	return EXIT_SUCCESS;
}
