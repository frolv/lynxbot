/*
 * command.h: macros to standardize command definitions
 * and command processing helper functions
 */

#ifndef _COMMAND_H
#define _COMMAND_H

#include "../lynxbot.h"

/* old macros */
#include <string>

#define CMDNAME(x) static const std::string CMDNAME = x
#define CMDDESCR(x) static const std::string CMDDESCR = x
#define CMDUSAGE(x) static const std::string CMDUSAGE = x

#define HELPMSG(NAME,USAGE,DESC) (std::string("usage: " + USAGE + " | "\
			+ NAME + " - " + DESC + " | read more: "\
			+ (std::string(BOT_WEBSITE) + "/manual/" + NAME\
			+ ".html")))

#define USAGEMSG(NAME,USAGE) (std::string(NAME + ": invalid syntax. usage: "\
			+ USAGE))
/* ----------- */

#define _CMDNAME(x) static const char *_CMDNAME = x
#define _CMDDESCR(x) static const char *_CMDDESCR = x
#define _CMDUSAGE(x) static const char *_CMDUSAGE = x

/* print usage synposis, brief description and link to man page */
#define _HELPMSG(buf,NAME,USAGE,DESC) _sprintf(buf, MAX_MSG, "usage: %s | "\
		"%s - %s | read more: %s/manual/%s.html", USAGE, NAME, DESC,\
		_BOT_WEBSITE, NAME)

/* print error with usage synopsis */
#define _USAGEMSG(buf,NAME,USAGE) _sprintf(buf, MAX_MSG, "%s: invalid syntax. "\
		"usage: %s", NAME, USAGE)

/* send user permission denied message */
#define PERM_DENIED(buf,NAME,CMD) _sprintf(buf, MAX_MSG, "/w %s %s: "\
		"permission denied", NAME, CMD)

/* argvcat: concatenate argv from optind to argc into buf */
void argvcat(char *buf, int argc, char **argv, int optind, int space);

#endif
