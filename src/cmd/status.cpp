#include <algorithm>
#include <cpr/cpr.h>
#include <tw/oauth.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("status");
/* description of the command */
CMDDESCR("set channel status");
/* command usage synopsis */
CMDUSAGE("status [-a] STATUS");

static const std::string TWITCH_API = "https://api.twitch.tv/kraken/channels/";

static std::string curr_status(const std::string &channel, cpr::Header &head,
		std::string &err);
static std::string set_status(const std::string &channel,
		const std::string &status, cpr::Header &head,
		std::string &err);

/* status: set channel status */
std::string CommandHandler::status(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string output, status, err;
	bool append;
	cpr::Header head{{ "Accept", "application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + m_token }};

	int opt;
	OptionParser op(c->fullCmd, "a");
	static struct OptionParser::option long_opts[] = {
		{ "append", NO_ARG, 'a' },
		{ "help", NO_ARG, 'h' },
		{ 0, 0, 0 }
	};

	append = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			append = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	/* get the current status if no arg provided or if appending */
	if (op.optind() == c->fullCmd.length() || append) {
		if ((status = curr_status(m_channel, head, err)).empty())
			return CMDNAME + ": " + err;
	}

	if (op.optind() == c->fullCmd.length()) {
		if (append)
			return c->cmd + ": no text to append";
		return "[STATUS] Current status for " + m_channel + " is \""
			+ status + "\".";
	}
	if (append)
		status += " ";
	status += c->fullCmd.substr(op.optind());

	if ((output = set_status(m_channel, status, head, err)).empty())
		return CMDNAME + ": " + err;
	return output;
}

/* curr_status: get current status of channel */
static std::string curr_status(const std::string &channel, cpr::Header &head,
		std::string &err)
{
	std::string out;
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;

	resp = cpr::Get(cpr::Url(TWITCH_API + channel), head);
	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			err = response["error"].asString();
			std::transform(err.begin(), err.end(),
					err.begin(), tolower);
			return "";
		}
		return response["status"].asString();
	}
	err = "could not parse channel data";
	return "";
}

/* curr_status: set new status of channel */
static std::string set_status(const std::string &channel,
		const std::string &status, cpr::Header &head,
		std::string &err)
{
	std::string out, content;
	cpr::Response resp;
	Json::Reader reader;
	Json::Value response;

	out = tw::pencode(status, " ");
	std::replace(out.begin(), out.end(), ' ', '+');
	content = "channel[status]=" + out;

	resp = cpr::Put(cpr::Url(TWITCH_API + channel), cpr::Body(content),
			head);
	if (reader.parse(resp.text, response)) {
		if (response.isMember("error")) {
			err = response["error"].asString();
			std::transform(err.begin(), err.end(),
					err.begin(), tolower);
			return "";
		}
		return "[STATUS] Channel status changed to \""
			+ response["status"].asString() + "\".";
	}
	err = "could not parse channel data";
	return "";
}
