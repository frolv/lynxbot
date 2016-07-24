#include <cpr/cpr.h>
#include <json/json.h>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("age");
/* description of the command */
CMDDESCR("check length of channel releationships");
/* command usage synopsis */
CMDUSAGE("age <-f|-s>");

static const std::string TWITCH_API = "https://api.twitch.tv/kraken";

static std::string parse_time(const std::string &ftime);

/* followage: check how long you have been following a channel */
std::string CommandHandler::age(struct cmdinfo *c)
{
	static cpr::Header head{{ "Accept","application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + m_token }};
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	std::string msg, url, out;

	int opt;
	OptionParser op(c->fullCmd, "fs");
	static struct OptionParser::option long_opts[] = {
		{ "follow", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "sub", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'f':
			url = TWITCH_API + "/users/" + c->nick
				+ "/follows/channels/" + m_channel;
			msg = "following";
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 's':
			url = TWITCH_API + "/channels/" + m_channel
				+ "/subscriptions/" + c->nick;
			msg = "subscribed to";
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() != c->fullCmd.length() || url.empty())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	resp = cpr::Get(cpr::Url(url), head);

	out = "@" + c->nick + ", ";
	if (!reader.parse(resp.text, response))
		return CMDNAME + ": could not parse response";
	if (!response.isMember("created_at"))
		return out + "you are not " + msg + " " + m_channel + ".";

	return out + "you have been " + msg + " " + m_channel + " for "
		+ parse_time(response["created_at"].asString()) + ".";
}

/* parse_time: extract time and date from ftime */
static std::string parse_time(const std::string &ftime)
{
	std::tm tm, curr;
	time_t t, now;
	std::ostringstream out;
	std::istringstream ss(ftime);

#ifdef __linux__
	ss.imbue(std::locale("en_US.utf-8"));
#endif
#ifdef _WIN32
	ss.imbue(std::locale("en-US"));
#endif
	ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
	t = std::mktime(&tm);

	now = time(NULL);
	curr = *std::gmtime(&now);
	now = std::mktime(&curr);

	out << utils::conv_time(now - t);
	out << " (since " << std::put_time(&tm, "%H:%M:%S UTC, %d %b %Y") << ")";

	return out.str();
}
