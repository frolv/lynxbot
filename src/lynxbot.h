/* lynxbot.h */

#ifndef _LYNXBOT_H
#define _LYNXBOT_H

#include <cstdio>

#define _BOT_NAME	"LynxBot"
#define _BOT_VER_MAJOR	"1"
#define _BOT_VER_MINOR	"4"
#define _BOT_VER_PATCH	"4"
#define _BOT_WEBSITE	"https://frolv.github.io/lynxbot"

#define BOT_VER "v" _BOT_VER_MAJOR "." _BOT_VER_MINOR "." _BOT_VER_PATCH

#ifdef __linux__
# define _sprintf snprintf
#endif
#ifdef _WIN32
# define _sprintf sprintf_s
#endif

#define WAIT_INPUT() while (getchar() != '\n');

const extern char *BOT_NAME;
const extern char *BOT_VERSION;
const extern char *BOT_WEBSITE;

#endif
