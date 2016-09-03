#include <cpr/cpr.h>
#include <json/json.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"

/* full name of the command */
CMDNAME("age");
/* description of the command */
CMDDESCR("check length of channel relationships");
/* command usage synopsis */
CMDUSAGE("age <-f|-s>");

static const char *TWITCH_API = "https://api.twitch.tv/kraken";

/* followage: check how long you have been following a channel */
int CmdHandler::age(char *out, struct command *c)
{
	static cpr::Header head{{ "Accept","application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + std::string(m_token) }};
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char url[MAX_MSG];
	const char *msg;

	int opt;
	static struct l_option long_opts[] = {
		{ "follow", NO_ARG, 'f' },
		{ "help", NO_ARG, 'h' },
		{ "sub", NO_ARG, 's' },
		{ 0, 0, 0 }
	};

	opt_init();
	url[0] = '\0';
	msg = NULL;
	while ((opt = l_getopt_long(c->argc, c->argv, "fs", long_opts)) != EOF) {
		switch (opt) {
		case 'f':
			snprintf(url, MAX_MSG, "%s/users/%s/follows/channels/%s",
					TWITCH_API, c->nick, m_channel);
			msg = "following";
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 's':
			snprintf(url, MAX_MSG, "%s/channels/%s/subscriptions/%s",
					TWITCH_API, m_channel, c->nick);
			msg = "subscribed to";
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (l_optind != c->argc || !url[0]) {
		USAGEMSG(out, CMDNAME, CMDUSAGE);
		return EXIT_FAILURE;
	}

	resp = cpr::Get(cpr::Url(url), head);

	if (!reader.parse(resp.text, response))
		snprintf(out, MAX_MSG, "%s: could not parse response", CMDNAME);
	else if (!response.isMember("created_at"))
		snprintf(out, MAX_MSG, "@%s, you are not %s %s.",
				c->nick, msg, m_channel);
	else
		snprintf(out, MAX_MSG, "@%s, you have been %s %s for %s.",
				c->nick, msg, m_channel,
				utils::parse_time(response["created_at"]
					.asString(), true).c_str());
	return EXIT_SUCCESS;
}
