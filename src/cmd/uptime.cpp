#include <cpr/cpr.h>
#include <ctime>
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

static const std::string UPTIME_API =
	"https://decapi.me/twitch/uptime.php?channel=";

static const std::string get_time(time_t t);

/* uptime: check how long channel has been live */
std::string CommandHandler::uptime(struct cmdinfo *c)
{
	std::string out;
	bool bot;
	cpr::Response resp;
	static const std::string channel =
		(char)toupper(m_channel[0]) + m_channel.substr(1);

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
			+ get_time(time(NULL) - m_evtp->init()) + ".";

	out = "@" + c->nick + ", ";
	resp = cpr::Get(cpr::Url(UPTIME_API + m_channel),
			cpr::Header{{ "Connection", "close" }});
	if (resp.text.substr(0, 7) == "Channel")
		return out + channel + " is not currently live.";
	else
		return out + channel + " has been live for " + resp.text + ".";
}

/* get_time: convert t to hours, minutes and seconds */
static const std::string get_time(time_t t)
{
	time_t h, m;
	std::string out;

	h = t / 3600;
	t %= 3600;
	m = t / 60;
	t %= 60;

	out += std::to_string(h) + " hour" + (h == 1 ? "" : "s") + ", ";
	out += std::to_string(m) + " minute" + (m == 1 ? "" : "s") + " and ";
	out += std::to_string(t) + " second" + (t == 1 ? "" : "s");
	return out;
}
