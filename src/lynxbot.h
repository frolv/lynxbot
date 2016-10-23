/*
 * lynxbot.h: bot information and general purpose definitions
 */

#ifndef LYNXBOT_H
#define LYNXBOT_H

#include <stdio.h>

/* general program information */
#define BOT_NAME	"LynxBot"
#define BOT_WEBSITE	"https://frolv.github.io/lynxbot"

/* version information */
#define _BOT_VER_MAJOR	"1"
#define _BOT_VER_MINOR	"4"
#define _BOT_VER_PATCH	"9"
#define _BOT_VER_SUFFIX	""

/* full version string */
#define BOT_VERSION "v" _BOT_VER_MAJOR "." _BOT_VER_MINOR "."\
	_BOT_VER_PATCH _BOT_VER_SUFFIX

#define __STDC_FORMAT_MACROS

#ifndef MAX_MSG
/* maximum length of chat message */
# define MAX_MSG 1024
#endif

/* maximum path length */
#define MAX_PATH 4096

/* buffer size for RSN fetching */
#define RSN_BUF 64

/* wait for user to press return */
#define WAIT_INPUT() while (getchar() != '\n');

#endif /* LYNXBOT_H */
