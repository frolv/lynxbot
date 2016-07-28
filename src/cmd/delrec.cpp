#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("delrec");
/* description of the command */
CMDDESCR("delete a recurring message");
/* command usage synopsis */
CMDUSAGE("$delrec ID");

/* delrec: delete a recurring message */
std::string CommandHandler::delrec(char *out, struct command *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string output = "@" + std::string(c->nick) + ", ";
	std::vector<std::string> argv;

	int opt, id;
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

	utils::split(c->fullCmd, ' ', argv);
	if (argv.size() != 2)
		return USAGEMSG(CMDNAME, CMDUSAGE);

	try {
		id = std::stoi(argv[1]);
	} catch (std::invalid_argument) {
		return c->cmd + ": invalid number -- '" + argv[1] + "'";
	} catch (std::out_of_range) {
		return CMDNAME + ": number too large";
	}

	if (!m_evtp->delMessage(id))
		return CMDNAME + "invalid ID provided";
	else
		return output + "recurring message " + std::to_string(id)
			+ " deleted.";
}
