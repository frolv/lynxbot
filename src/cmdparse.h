#ifndef _CMDPARSE_H
#define _CMDPARSE_H

#include "CommandHandler.h"

int parse_cmd(char *cmdstr, struct CommandHandler::command *c);
void free_cmd(struct CommandHandler::command *c);
char *cmderr();

#endif
