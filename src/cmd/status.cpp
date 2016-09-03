#include <algorithm>
#include <cpr/cpr.h>
#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CmdHandler.h"
#include "../option.h"
#include "../sed.h"

#define MAX_URL 128

/* full name of the command */
CMDNAME("status");
/* description of the command */
CMDDESCR("set channel status");
/* command usage synopsis */
CMDUSAGE("status [-a] [-s SEDCMD] [STATUS]");

static const char *TWITCH_API = "https://api.twitch.tv/kraken/channels/";

static int curr_status(char *buf, const char *channel, const cpr::Header *head);
static int set_status(char *out, const char *channel, const char *status,
		const cpr::Header *head);

/* status: set channel status */
int CmdHandler::status(char *out, struct command *c)
{
	const cpr::Header head{{ "Accept", "application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + std::string(m_token) }};

	char buf[MAX_MSG];
	char *sedcmd;
	int append;

	int opt;
	static struct l_option long_opts[] = {
		{ "append", NO_ARG, 'a' },
		{ "help", NO_ARG, 'h' },
		{ "sed", REQ_ARG, 's' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return EXIT_FAILURE;
	}

	opt_init();
	append = 0;
	sedcmd = NULL;
	buf[0] = '\0';
	while ((opt = l_getopt_long(c->argc, c->argv, "as:", long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			append = 1;
			break;
		case 'h':
			HELPMSG(out, CMDNAME, CMDUSAGE, CMDDESCR);
			return EXIT_SUCCESS;
		case 's':
			sedcmd = l_optarg;
			break;
		case '?':
			snprintf(out, MAX_MSG, "%s", l_opterr());
			return EXIT_FAILURE;
		default:
			return EXIT_FAILURE;
		}
	}

	if (append && sedcmd) {
		snprintf(out, MAX_MSG, "%s: cannot combine -s and -a",
				c->argv[0]);
		return EXIT_FAILURE;
	}

	/* get the current status if no arg provided, appending or sed */
	if (l_optind == c->argc || append || sedcmd) {
		if (!curr_status(buf, m_channel, &head)) {
			snprintf(out, MAX_MSG, "%s: %s", c->argv[0], buf);
			return EXIT_FAILURE;
		}
	}

	if (l_optind == c->argc) {
		if (append) {
			snprintf(out, MAX_MSG, "%s: no text to append",
					c->argv[0]);
			return EXIT_FAILURE;
		} else if (sedcmd) {
			if (!sed(buf, MAX_MSG, buf, sedcmd)) {
				snprintf(out, MAX_MSG, "%s: %s",
						c->argv[0], buf);
				return EXIT_FAILURE;
			}
		} else {
			snprintf(out, MAX_MSG, "[STATUS] Current status for %s "
					"is \"%s\".", m_channel, buf);
			return EXIT_SUCCESS;
		}
	} else if (sedcmd) {
		snprintf(out, MAX_MSG, "%s: cannot provide status with -s",
				c->argv[0]);
		return EXIT_FAILURE;
	}

	if (append)
		strcat(buf, " ");
	argvcat(buf + strlen(buf), c->argc, c->argv, l_optind, 1);

	return set_status(out, m_channel, buf, &head);
}

/* curr_status: get current status of channel */
static int curr_status(char *buf, const char *channel, const cpr::Header *head)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char url[MAX_URL];
	char *s;

	snprintf(url, MAX_URL, "%s%s", TWITCH_API, channel);
	resp = cpr::Get(cpr::Url(url), *head);

	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			snprintf(buf, MAX_MSG, "%s",
					response["error"].asCString());
			for (s = buf; *s; ++s)
				*s = tolower(*s);
			return 0;
		}
		snprintf(buf, MAX_MSG, "%s", response["status"].asCString());
		return 1;
	}
	snprintf(buf, MAX_MSG, "could not parse channel data", CMDNAME);
	return 0;
}

/* curr_status: set new status of channel */
static int set_status(char *out, const char *channel, const char *status,
		const cpr::Header *head)
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;
	char body[MAX_MSG];
	char url[MAX_URL];
	char *s;

	snprintf(body, MAX_MSG, "channel[status]=%s",
			tw::pencode(status, " ").c_str());
	for (s = body; *s; ++s) {
		if (*s == ' ')
			*s = '+';
	}

	snprintf(url, MAX_URL, "%s%s", TWITCH_API, channel);
	resp = cpr::Put(cpr::Url(url), cpr::Body(body), *head);

	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			snprintf(out, MAX_MSG, "%s: %s", CMDNAME,
					response["error"].asCString());
			for (s = out; *s; ++s)
				*s = tolower(*s);
			return EXIT_FAILURE;
		}
		snprintf(out, MAX_MSG, "[STATUS] Channel status changed to "
				"\"%s\".", response["status"].asCString());
		return EXIT_SUCCESS;
	}
	snprintf(out, MAX_MSG, "%s: could not parse channel data", CMDNAME);
	return EXIT_FAILURE;
}
