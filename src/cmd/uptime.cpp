#include <cpr/cpr.h>
#include <ctime>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../timers.h"

/* full name of the command */
CMDNAME("uptime");
/* description of the command */
CMDDESCR("check how long channel has been live");
/* command usage synopsis */
CMDUSAGE("$uptime [-b]");

static int chan_uptime(char *out, const char *channel);

/* uptime: check how long channel has been live */
int CmdHandler::uptime(char *out, struct command *c)
{
	int opt, bot;
	static struct l_option long_opts[] = {
		{ "bot", NO_ARG, 'b' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	bot = 0;
	opt_init();
	while ((opt = l_getopt_long(c->argc, c->argv, "b", long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			bot = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	if (bot) {
		snprintf(out, MAX_MSG, "[UPTIME] %s has been running for %s.",
				m_name, utils::conv_time(bot_uptime()).c_str());
		return EXIT_SUCCESS;
	}

	return chan_uptime(out, m_channel);
}

/* channel_uptime: get how long channel has been streaming */
static int chan_uptime(char *out, const char *channel)
{
	time_t up;

	if ((up = channel_uptime()) == 0)
		snprintf(out, MAX_MSG, "[UPTIME] %s is not currently live.",
				channel);
	else if (up == -1)
		snprintf(out, MAX_MSG, "%s: Twitch authorization required",
				CMDNAME);
	else
		snprintf(out, MAX_MSG, "[UPTIME] %s has been live "
				"for %s.", channel,
				utils::conv_time(channel_uptime()).c_str());
	return EXIT_SUCCESS;
}
