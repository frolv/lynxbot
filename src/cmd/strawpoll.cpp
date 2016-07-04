#include <cpr/cpr.h>
#include <json/json.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../OptionParser.h"

/* full name of the command */
CMDNAME("strawpoll");
/* description of the command */
CMDDESCR("create polls");
/* command usage synopsis */
CMDUSAGE("$strawpoll [-bcm] QUESTION | OPTION1 | OPTION2...");

static const std::string STRAWPOLL_HOST = "https://strawpoll.me";
static const std::string STRAWPOLL_API = "/api/v2/polls";
static bool binary, captcha, multi;

static std::string create_poll(const std::vector<std::string> &tokens,
		std::string &err);

/* strawpoll: create polls */
std::string CommandHandler::strawpoll(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return NO_PERM(c->nick, c->cmd);

	std::string err;
	std::vector<std::string> tokens;

	int opt;
	OptionParser op(c->fullCmd, "bcm");
	static struct OptionParser::option long_opts[] = {
		{ "binary", NO_ARG, 'b' },
		{ "captcha", NO_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "multi", NO_ARG, 'm' },
		{ 0, 0, 0 }
	};

	binary = captcha = multi = false;
	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			binary = true;
			break;
		case 'c':
			captcha = true;
			break;
		case 'h':
			return HELPMSG(CMDNAME, CMDUSAGE, CMDDESCR);
		case 'm':
			multi = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return USAGEMSG(CMDNAME, CMDUSAGE);

	utils::split(c->fullCmd.substr(op.optind()), '|', tokens);
	if (binary && tokens.size() != 1)
		return c->cmd + ": cannot provide options for binary poll";
	if (!binary && tokens.size() < 3)
		return c->cmd + ": poll must have a question and at least "
			"two answers";

	if ((m_activePoll = create_poll(tokens, err)).empty())
		return c->cmd + ": " + err;
	return "[STRAWPOLL] " + STRAWPOLL_HOST + "/" + m_activePoll;
}

/* create_poll: create a strawpoll, return id */
static std::string create_poll(const std::vector<std::string> &tokens,
		std::string &err)
{
	/* json values to hold created poll, poll options and http response */
	Json::Value poll, options(Json::arrayValue), response;
	Json::FastWriter fw;
	Json::Reader reader;
	cpr::Response resp;
	std::string content;
	size_t i;

	if (binary) {
		options.append("yes");
		options.append("no");
	} else {
		for (i = 1; i < tokens.size(); ++i)
			options.append(tokens[i]);
	}

	/* populate the poll json */
	poll["title"] = tokens[0];
	poll["options"] = options;
	poll["captcha"] = captcha;
	poll["multi"] = multi;

	/* format and post the poll */
	content = fw.write(poll);
	resp = cpr::Post(cpr::Url(STRAWPOLL_HOST + STRAWPOLL_API),
			cpr::Body(content), cpr::Header{
			{ "Connection", "close" },
			{ "Content-Type", "application/json" },
			{ "Content-Length", std::to_string(content.length())}});

	/* read id from response */
	if (reader.parse(resp.text, response)) {
		if (!response.isMember("id")) {
			err = "poll could not be created";
			return "";
		}
		return response["id"].asString();
	}
	err = "could not parse response";
	return "";
}
