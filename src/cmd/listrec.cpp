#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("listrec");
/* description of the command */
CMDDESCR("show all recurring messages");
/* command usage synopsis */
CMDUSAGE("$listrec");

/* listrec: show all recurring messages */
std::string CommandHandler::listrec(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	return m_evtp->messageList();
}
