#include <algorithm>
#include "command.h"
#include "../CommandHandler.h"
#include "../ExpressionParser.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("calc");
/* description of the command */
CMDDESCR("perform basic calculations");
/* command usage synopsis */
CMDUSAGE("$calc EXPR");

/* calc: perform basic calculations */
std::string CommandHandler::calc(struct cmdinfo *c)
{
	std::string expr;
	std::ostringstream result;
	double res;

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

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	expr = c->fullCmd.substr(op.optind());
	/* remove all whitespace */
	expr.erase(std::remove_if(expr.begin(), expr.end(), isspace), expr.end());

	try {
		ExpressionParser exp(expr);
		exp.tokenizeExpr();
		res = exp.eval();
		result << res;
	} catch (std::runtime_error &e) {
		return CMDNAME + ": " + e.what();
	}
	if (result.str() == "inf" || result.str() == "-nan(ind)")
		return CMDNAME + ": division by 0";

	return "[CALC] " + result.str();
}
