#include <cpr/cpr.h>
#include <ctime>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("uptime");
/* description of the command */
CMDDESCR("check how long channel has been live");
/* command usage synopsis */
CMDUSAGE("$uptime [-b]");

static const char *UPTIME_API = "https://api.twitch.tv/kraken/streams/";

static void channel_uptime(char *out, const char *channel);

/* uptime: check how long channel has been live */
std::string CommandHandler::uptime(char *out, struct command *c)
{
	int opt, bot;
	static struct option long_opts[] = {
		{ "bot", NO_ARG, 'b' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	bot = 0;
	opt_init();
	while ((opt = getopt_long(c->argc, c->argv, "b", long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			bot = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return "";
	}

	if (bot) {
		_sprintf(out, MAX_MSG, "[UPTIME] %s has been running for %s.",
				m_name, + utils::conv_time((time(NULL)
						- m_evtp->init())).c_str());
		return "";
	}

	channel_uptime(out, m_channel);
	return "";
}

/* channel_uptime: get how long channel has been streaming */
static void channel_uptime(char *out, const char *channel)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value val;
	char url[MAX_URL];

	_sprintf(url, MAX_URL, "%s%s", UPTIME_API, channel);
	resp = cpr::Get(cpr::Url(url), cpr::Header{{ "Connection", "close" }});
	if (!reader.parse(resp.text, val))
		_sprintf(out, MAX_MSG, "%s: could not parse response", CMDNAME);
	else if (val["stream"].isNull())
		_sprintf(out, MAX_MSG, "[UPTIME] %s is not currently live.",
				channel);
	else
		_sprintf(out, MAX_MSG, "[UPTIME] %s has been live for %s.",
				channel, utils::parse_time(
					val["stream"]["created_at"].asString(),
					false).c_str());
}
