#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("showrec");
/* description of the command */
CMDDESCR("show recurring messages");
/* command usage synopsis */
CMDUSAGE("$showrec [ID]");

/* listrec: show recurring messages */
std::string CommandHandler::showrec(struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string num;
	size_t id;

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
		return m_evtp->messageList();

	if ((num = c->fullCmd.substr(op.optind())).find(' ') != std::string::npos)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	/* show a single message */
	try {
		if ((id = std::stoi(num)) < 1 || id > 5
				|| m_evtp->messages()->size() < id)
			return CMDNAME + ": recurring message "
				+ std::to_string(id) + " doesn't exist";
	} catch (std::invalid_argument) {
		return CMDNAME + ": invalid number -- '" + num + "'";
	} catch (std::out_of_range) {
		return CMDNAME + ": number too large";
	}

	return m_evtp->message(id - 1);
}
