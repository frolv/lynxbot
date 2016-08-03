#include <cpr/cpr.h>
#include <json/json.h>
#include <utils.h>
#include "command.h"
#include "../CommandHandler.h"
#include "../option.h"

/* full name of the command */
_CMDNAME("strawpoll");
/* description of the command */
_CMDDESCR("create polls");
/* command usage synopsis */
_CMDUSAGE("$strawpoll [-bcm] QUESTION | OPTION1 | OPTION2...");

static const char *STRAWPOLL_HOST = "https://strawpoll.me";
static const char *STRAWPOLL_API = "/api/v2/polls";

/* create a yes/no poll */
static bool binary;
/* whether to use captcha */
static bool captcha;
/* whether to allow multiple choices */
static bool multi;

static void create_poll(char *out, char *pollbuf, struct command *c);

/* strawpoll: create polls */
std::string CommandHandler::strawpoll(char *out, struct command *c)
{
	int opt;
	static struct option long_opts[] = {
		{ "binary", NO_ARG, 'b' },
		{ "captcha", NO_ARG, 'c' },
		{ "help", NO_ARG, 'h' },
		{ "multi", NO_ARG, 'm' },
		{ 0, 0, 0 }
	};

	if (!P_ALMOD(c->privileges)) {
		PERM_DENIED(out, c->nick, c->argv[0]);
		return "";
	}

	opt_init();
	binary = captcha = multi = false;
	while ((opt = getopt_long(c->argc, c->argv, "bcm", long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			binary = true;
			break;
		case 'c':
			captcha = true;
			break;
		case 'h':
			_HELPMSG(out, _CMDNAME, _CMDUSAGE, _CMDDESCR);
			return "";
		case 'm':
			multi = true;
			break;
		case '?':
			_sprintf(out, MAX_MSG, "%s", opterr());
			return "";
		default:
			return "";
		}
	}

	if (optind == c->argc) {
		_USAGEMSG(out, _CMDNAME, _CMDUSAGE);
		return "";
	}

	create_poll(out, m_poll, c);
	/* if (binary && tokens.size() != 1) */
	/* 	return c->cmd + ": cannot provide options for binary poll"; */
	/* if (!binary && tokens.size() < 3) */
	/* 	return c->cmd + ": poll must have a question and at least " */
	/* 		"two answers"; */

	/* std::string poll; */
	/* if ((poll = create_poll(tokens, err)).empty()) { */
	/* 	m_poll[0] = '\0'; */
	/* 	return c->cmd + ": " + err; */
	/* } */
	/* strcpy(m_poll, poll.c_str()); */
	/* return "[STRAWPOLL] " + STRAWPOLL_HOST + "/" + poll; */
	return "";
}

/* create_poll: create a strawpoll, return id */
static void create_poll(char *out, char *pollbuf, struct command *c)
{
	/* json values to hold created poll, poll options and http response */
	Json::Value poll, options(Json::arrayValue), response;
	Json::FastWriter fw;
	Json::Reader reader;
	cpr::Response resp;
	char buf[MAX_MSG];
	char *quest, *s, *t;

	argvcat(buf, c->argc, c->argv, optind, 1);
	quest = buf;
	if (!(s = strchr(quest, '|'))) {
		if (!binary) {
			_sprintf(out, MAX_MSG, "%s: poll must have a question "
					"and at least two answers", c->argv[0]);
			return;
		}
	} else {
		*s = '\0';
		if (binary) {
			_sprintf(out, MAX_MSG, "%s: cannot provide answers "
					"for binary poll", c->argv[0]);
			return;
		}
	}

	if (binary) {
		options.append("yes");
		options.append("no");
	} else {
		for (++s; *s && (t = strchr(s, '|')); s = t + 1) {
			*t = '\0';
			options.append(s);
		}
		if (*s)
			options.append(s);
	}

	if (options.size() < 2) {
		_sprintf(out, MAX_MSG, "%s: poll must have a question "
				"and at least two answers", c->argv[0]);
		return;
	}

	/* populate the poll json */
	poll["title"] = quest;
	poll["options"] = options;
	poll["captcha"] = captcha;
	poll["multi"] = multi;

	/* format and post the poll */
	_sprintf(buf, MAX_MSG, "%s%s", STRAWPOLL_HOST, STRAWPOLL_API);
	resp = cpr::Post(cpr::Url(buf), cpr::Body(fw.write(poll)), cpr::Header{
			{ "Connection", "close" },
			{ "Content-Type", "application/json" }});

	/* read id from response */
	if (reader.parse(resp.text, response)) {
		if (!response.isMember("id")) {
			_sprintf(out, MAX_MSG, "%s: poll could not be created",
					c->argv[0]);
			return;
		}
		_sprintf(pollbuf, MAX_LEN, "%d", response["id"].asInt());
		_sprintf(out, MAX_MSG, "[STRAWPOLL] %s/%s",
				STRAWPOLL_HOST, pollbuf);
		return;
	}
	_sprintf(out, MAX_MSG, "%s: could not parse response", c->argv[0]);
}
