/* command.h: macros to standardize command definitions */

#include <string>
#include "../version.h"

#define CMDNAME(x) const static std::string CMDNAME = x
#define CMDDESCR(x) const static std::string CMDDESCR = x
#define CMDUSAGE(x) const static std::string CMDUSAGE = x

#define HELPMSG(NAME,USAGE,DESC) (std::string("usage: " + USAGE + " | "\
			+ NAME + " - " + DESC + " | read more: "\
			+ (std::string(BOT_WEBSITE) + "/manual/" + NAME\
			+ ".html")))
