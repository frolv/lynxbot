/*
 * command.h: macros to standardize command definitions
 * and command processing helper functions
 */

#ifndef _COMMAND_H
#define _COMMAND_H

#include "../lynxbot.h"
#include "../permissions.h"

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#define CMDNAME(x) static const char *CMDNAME = x
#define CMDDESCR(x) static const char *CMDDESCR = x
#define CMDUSAGE(x) static const char *CMDUSAGE = x

/* print usage synposis, brief description and link to man page */
#define HELPMSG(buf,NAME,USAGE,DESC) snprintf(buf, MAX_MSG, "usage: %s | "\
		"%s - %s | read more: %s/manual/%s.html", USAGE, NAME, DESC,\
		BOT_WEBSITE, NAME)

/* print error with usage synopsis */
#define USAGEMSG(buf,NAME,USAGE) snprintf(buf, MAX_MSG, "%s: invalid syntax. "\
		"usage: %s", NAME, USAGE)

/* send user permission denied message */
#define PERM_DENIED(buf,NAME,CMD) snprintf(buf, MAX_MSG, "/w %s %s: "\
		"permission denied", NAME, CMD)

struct command {
	char *nick;		/* name of command user */
	int argc;		/* number of arguments */
	char **argv;		/* array of arguments */
	perm_t privileges;	/* user privileges */
};

#ifdef __cplusplus
extern "C" {
#endif

/* argvcat: concatenate argv from start to argc into buf */
void argvcat(char *buf, int argc, char **argv, int start, int space);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
