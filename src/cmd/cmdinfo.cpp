#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("cmdinfo");
/* description of the command */
CMDDESCR("show information about a custom command");
/* command usage synopsis */
CMDUSAGE("$cmdinfo [-acCmu] CMD");

/* cmdinfo: show information about a custom command */
std::string CommandHandler::cmdinf(struct cmdinfo *c)
{
	int opt;
	OptionParser op(c->fullCmd, "acCmu");
	static struct OptionParser::option long_opts[] = {
		{ "atime", NO_ARG, 'a' },
		{ "ctime", NO_ARG, 'c' },
		{ "creator", NO_ARG, 'C' },
		{ "help", NO_ARG, 'h' },
		{ "mtime", NO_ARG, 'm' },
		{ "uses", NO_ARG, 'u' },
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
	return "";
}
