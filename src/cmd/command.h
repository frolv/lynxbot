/* command.h: macros to standardize command definitions */

#ifndef _COMMAND_H
#define _COMMAND_H

#include <string>
#include "../lynxbot.h"

/* old formats */
#define CMDNAME(x) const static std::string CMDNAME = x
#define CMDDESCR(x) const static std::string CMDDESCR = x
#define CMDUSAGE(x) const static std::string CMDUSAGE = x

#define HELPMSG(NAME,USAGE,DESC) (std::string("usage: " + USAGE + " | "\
			+ NAME + " - " + DESC + " | read more: "\
			+ (std::string(BOT_WEBSITE) + "/manual/" + NAME\
			+ ".html")))

#define USAGEMSG(NAME,USAGE) (std::string(NAME + ": invalid syntax. usage: "\
			+ USAGE))
/* ----------- */

#define _CMDNAME(x) const static char *_CMDNAME = x
#define _CMDDESCR(x) const static char *_CMDDESCR = x
#define _CMDUSAGE(x) const static char *_CMDUSAGE = x

/* print usage synposis, brief description and link to man page */
#define _HELPMSG(buf,NAME,USAGE,DESC) _sprintf(buf, MAX_MSG, "usage: %s | "\
		"%s - %s | read more: %s/manual/%s.html", USAGE, NAME, DESC,\
		BOT_WEBSITE, NAME)

/* print error with usage synopsis */
#define _USAGEMSG(buf,NAME,USAGE) _sprintf(buf, MAX_MSG, "%s: invalid syntax. "\
		"usage: %s", NAME, USAGE)

#endif
