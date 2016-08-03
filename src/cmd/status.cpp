#include <algorithm>
#include <cpr/cpr.h>
#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

#define MAX_URL 128

/* full name of the command */
_CMDNAME("status");
/* description of the command */
_CMDDESCR("set channel status");
/* command usage synopsis */
_CMDUSAGE("status [-a] [STATUS]");

static const char *TWITCH_API = "https://api.twitch.tv/kraken/channels/";

static int curr_status(char *buf, const char *channel, const cpr::Header *head);
static void set_status(char *out, const char *channel, const char *status,
		const cpr::Header *head);

/* status: set channel status */
std::string CommandHandler::status(char *out, struct command *c)
{
	const cpr::Header head{{ "Accept", "application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + std::string(m_token) }};

	char buf[MAX_MSG];
	int append;

	int opt;
	static struct option long_opts[] = {
		{ "append", NO_ARG, 'a' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	append = 0;
	buf[0] = '\0';
	while ((opt = getopt_long(c->argc, c->argv, "a", long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			append = 1;
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	/* get the current status if no arg provided or if appending */
	if (optind == c->argc || append) {
		if (!curr_status(buf, m_channel, &head)) {
			_sprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
			return "";
		}
	}

	if (optind == c->argc) {
		if (append)
			_sprintf(out, MAX_MSG, "%s: no text to append",
					c->argv[0]);
		else
			_sprintf(out, MAX_MSG, "[STATUS] Current status for %s "
					"is \"%s\".", m_channel, buf);
		return "";
	}

	if (append)
		strcat(buf, " ");
	argvcat(buf + strlen(buf), c->argc, c->argv, optind, 1);
	set_status(out, m_channel, buf, &head);

	return "";
}

/* curr_status: get current status of channel */
static int curr_status(char *buf, const char *channel, const cpr::Header *head)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char url[MAX_URL];
	char *s;

	_sprintf(url, MAX_URL, "%s%s", TWITCH_API, channel);
	resp = cpr::Get(cpr::Url(url), *head);

	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			_sprintf(buf, MAX_MSG, "%s",
					response["error"].asCString());
			for (s = buf; *s; ++s)
				*s = tolower(*s);
			return 0;
		}
		_sprintf(buf, MAX_MSG, "%s", response["status"].asCString());
		return 1;
	}
	_sprintf(buf, MAX_MSG, "could not parse channel data");
	return 0;
}

/* curr_status: set new status of channel */
static void set_status(char *out, const char *channel, const char *status,
		const cpr::Header *head)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char body[MAX_MSG];
	char url[MAX_URL];
	char *s;

	_sprintf(body, MAX_MSG, "channel[status]=%s",
			tw::pencode(status, " ").c_str());
	for (s = body; *s; ++s) {
		if (*s == ' ')
			*s = '+';
	}

	_sprintf(url, MAX_URL, "%s%s", TWITCH_API, channel);
	resp = cpr::Put(cpr::Url(url), cpr::Body(body), *head);

	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			_sprintf(out, MAX_MSG, "%s: %s", _CMDNAME,
					response["error"].asCString());
			for (s = out; *s; ++s)
				*s = tolower(*s);
			return;
		}
		_sprintf(out, MAX_MSG, "[STATUS] Channel status changed to "
				"\"%s\".", response["status"].asCString());
		return;
	}
	_sprintf(out, MAX_MSG, "could not parse channel data");
}
