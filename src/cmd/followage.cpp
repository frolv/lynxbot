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
CMDNAME("followage");
/* description of the command */
CMDDESCR("check how long you have been following a channel");
/* command usage synopsis */
CMDUSAGE("$followage");

static const std::string TWITCH_API = "https://api.twitch.tv/kraken/users/";
static const std::string TWITCH_FOLLOWS = "/follows/channels/";

static std::string parse_time(const std::string &ftime);

/* followage: check how long you have been following a channel */
std::string CommandHandler::followage(struct cmdinfo *c)
{
	static cpr::Header head{{ "Accept","application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + m_token }};
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	std::string url, out;

	int opt;
	OptionParser op(c->fullCmd, "");
	static struct OptionParser::option long_opts[] = {
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
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

	url = TWITCH_API + c->nick + TWITCH_FOLLOWS + m_channel;
	resp = cpr::Get(cpr::Url(url), head);

	out = "@" + c->nick + ", ";
	if (!reader.parse(resp.text, response))
		return CMDNAME + ": could not parse response";
	if (!response.isMember("created_at"))
		return out + "you are not following " + m_channel + ".";

	return out + "you have been following " + m_channel + " for "
		+ parse_time(response["created_at"].asString()) + ".";
}

/* parse_time: extract time and date from ftime */
static std::string parse_time(const std::string &ftime)
{
	std::tm tm;
	time_t t;
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

	out << utils::conv_time(time(NULL) - t);
	out << " (since " << std::put_time(&tm, "%H:%M:%S, %d %b %Y") << ")";

	return out.str();
}
