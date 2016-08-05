/*
 * cmdparse.h: functions to break up a chat message
 * into an array of arguments for a command
 */

#ifndef _CMDPARSE_H
#define _CMDPARSE_H

#include "cmd/command.h"

#ifdef __cplusplus
extern "C" {
#endif

/* parse_cmd: split cmdstr into argv of c */
int parse_cmd(char *cmdstr, struct command *c);

/* free_cmd: free allocated memory in c */
void free_cmd(struct command *c);

/* cmderr: return parsing error */
char *cmderr();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
