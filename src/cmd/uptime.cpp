#include <cpr/cpr.h>
#include <ctime>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"
#include "../version.h"

/* full name of the command */
CMDNAME("uptime");
/* description of the command */
CMDDESCR("check how long channel has been live");
/* command usage synopsis */
CMDUSAGE("$uptime [-b]");

static const std::string UPTIME_API = "https://api.twitch.tv/kraken/streams/";

static std::string channel_uptime(const std::string &channel);

/* uptime: check how long channel has been live */
std::string CommandHandler::uptime(struct cmdinfo *c)
{
	std::string out;
	bool bot;

	int opt;
	OptionParser op(c->fullCmd, "b");
	static struct OptionParser::option long_opts[] = {
		{ "bot", NO_ARG, 'b' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	bot = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			bot = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	if (bot)
		return m_name + " has been running for "
			+ utils::conv_time((time(NULL) - m_evtp->init())) + ".";

	return "[UPTIME] " + channel_uptime(m_channel);
}

/* channel_uptime: get how long channel has been streaming */
static std::string channel_uptime(const std::string &channel)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value val;

	resp = cpr::Get(cpr::Url(UPTIME_API + channel),
			cpr::Header{{ "Connection", "close" }});
	if (!reader.parse(resp.text, val))
		return "could not parse response";

	if (val["stream"].isNull())
		return channel + " is not currently live.";

	return channel + " has been live for "
		+ utils::parse_time(val["stream"]["created_at"].asString(),
				false) + ".";
}
