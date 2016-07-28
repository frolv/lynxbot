#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("selection-wheel");
/* description of the command */
CMDDESCR("select items from various categories");
/* command usage synopsis */
CMDUSAGE("$WHEELCMD CATEGORY");

/* wheel: select items from various categories */
std::string CommandHandler::wheel(struct command *c)
{
	std::string cmd, out;

	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'h':
			return HELPMSG(CMDNAME, m_wheel.usage(),
					CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return m_wheel.name() + ": " + m_wheel.desc()
			+ " " + m_wheel.usage();

	/* check if category is valid */
	if (!m_wheel.valid((cmd = c->fullCmd.substr(op.optind())))
				&& cmd != "check")
		return USAGEMSG(c->cmd, m_wheel.usage());

	out = "@" + std::string(c->nick) + ", ";
	if (cmd == "check") {
		/* return the current selection */
		out += m_wheel.ready(c->nick)
			? "you are not currently assigned anything."
			: "you are currently assigned "
			+ m_wheel.selection(c->nick) + ".";
	} else if (!m_wheel.ready(c->nick)) {
		out += "you have already been assigned something!";
	} else {
		/* make a new selection */
		out += "your entertainment for tonight is "
			+ m_wheel.choose(c->nick, cmd) + ".";
	}

	return out;
}
