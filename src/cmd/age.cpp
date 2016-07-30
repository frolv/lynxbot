#include <cpr/cpr.h>
#include <json/json.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("age");
/* description of the command */
_CMDDESCR("check length of channel relationships");
/* command usage synopsis */
_CMDUSAGE("age <-f|-s>");

static const char *TWITCH_API = "https://api.twitch.tv/kraken";

/* followage: check how long you have been following a channel */
std::string CommandHandler::age(char *out, struct command *c)
{
	static cpr::Header head{{ "Accept","application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + std::string(m_token) }};
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char url[MAX_MSG];
	const char *msg;

	int opt;
	static struct option long_opts[] = {
		{ "follow", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "sub", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	opt_init();
	url[0] = '\0';
	while ((opt = getopt_long(c->argc, c->argv, "fs", long_opts)) != EOF) {
		switch (opt) {
		case 'f':
			_sprintf(url, MAX_MSG, "%s/users/%s/follows/channels/%s",
					TWITCH_API, c->nick, m_channel);
			msg = "following";
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 's':
			_sprintf(url, MAX_MSG, "%s/channels/%s/subscriptions/%s",
					TWITCH_API, m_channel, c->nick);
			msg = "subscribed to";
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind != c->argc || !url[0]) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	resp = cpr::Get(cpr::Url(url), head);

	if (!reader.parse(resp.text, response))
		_sprintf(out, MAX_MSG, "%s: could not parse response", _CMDNAME);
	else if (!response.isMember("created_at"))
		_sprintf(out, MAX_MSG, "@%s, you are not %s %s.",
				c->nick, msg, m_channel);
	else
		_sprintf(out, MAX_MSG, "@%s, you have been %s %s for %s.",
				c->nick, msg, m_channel,
				utils::parse_time(response["created_at"]
					.asString(), true).c_str());
	return "";
}
