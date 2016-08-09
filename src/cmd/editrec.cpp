#include <string.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../sed.h"
#include "../stringparse.h"

/* full name of the command */
CMDNAME("editrec");
/* description of the command */
CMDDESCR("modify a recurring message");
/* command usage synopsis */
CMDUSAGE("editrec [-c INTERVAL] [-s SEDCMD] ID [MSG]");

/* editrec: modify a recurring message */
int CmdHandler::editcom(char *out, struct command *c)
{
	return EXIT_SUCCESS;
}
