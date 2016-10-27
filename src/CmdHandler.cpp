#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <regex>
#include <utils.h>
#include "cmdparse.h"
#include "CmdHandler.h"
#include "lynxbot.h"

CmdHandler::CmdHandler(const char *name, const char *channel, const char *token,
		Moderator *mod, URLParser *urlp, EventManager *evt,
		Giveaway *giv, ConfigReader *cfgr, tw::Authenticator *auth)
	: bot_name(name), bot_channel(channel), twitch_token(token),
	moderator(mod), urlparser(urlp), custom_cmds(NULL), evtman(evt),
	giveaway(giv), cfg(cfgr), tw_auth(auth), count_active(false), gen(rd()),
	return_status(EXIT_SUCCESS)
{
	add_commands();
	poll[0] = '\0';

	custom_cmds = new CustomHandler(&default_cmds, &cooldowns,
			swheel.cmd(), name, channel);
	if (!custom_cmds->active()) {
		fprintf(stderr, "Custom commands will be "
				"disabled for this session\n");
		WAIT_INPUT();
	}

	/* read all responses from file */
	if ((responding = utils::read_json("responses.json", responses))) {
		/* add response cooldowns to TimerManager */
		for (auto &val : responses["responses"])
			cooldowns.add('_' + val["name"].asString(),
				val["cooldown"].asInt());
	} else {
		fprintf(stderr, "Failed to read responses.json. "
				"Responses disabled for this session\n");
		WAIT_INPUT();
	}

	/* read extra 8ball responses */
	utils::split(cfg->get("extra8ballresponses"), '\n', eightball_responses);

	/* read fashion.json */
	if (!utils::read_json("fashion.json", fashion))
		fprintf(stderr, "Could not read fashion.json\n");

	populate_help();
}

CmdHandler::~CmdHandler()
{
	delete custom_cmds;
}

/* process_cmd: run message as a bot command and store output in out */
void CmdHandler::process_cmd(char *out, char *nick, char *cmdstr, perm_t p)
{
	struct command c;

	c.nick = nick;
	c.privileges = p;
	if (!parse_cmd(cmdstr, &c)) {
		snprintf(out, MAX_MSG, "%s", cmderr());
		return;
	}

	switch (source(c.argv[0])) {
	case DEFAULT:
		process_default(out, &c);
		break;
	case CUSTOM:
		process_custom(out, &c);
		break;
	default:
		snprintf(out, MAX_MSG, "/w %s not a bot command: %s",
				nick, c.argv[0]);
		break;
	}
	free_cmd(&c);
}

/* process_resp: test a message against auto response regexes */
int CmdHandler::process_resp(char *out, char *msg, char *nick)
{
	const std::string message(msg);
	std::regex respreg;
	std::smatch match;
	std::string name, regex;

	*out = '\0';
	if (!responding)
		return 0;

	/* test the message against all response regexes */
	for (auto &val : responses["responses"]) {
		name = '_' + val["name"].asString();
		regex = val["regex"].asString();

		try {
			respreg = std::regex(regex, std::regex::icase);
		} catch (std::regex_error) {
			fprintf(stderr, "invalid regex: %s\n", regex.c_str());
			continue;
		}
		if (std::regex_search(message.begin(), message.end(), match,
				respreg) && cooldowns.ready(name)) {
			cooldowns.set_used(name);
			snprintf(out, MAX_MSG, "@%s, %s", nick,
					val["response"].asCString());
			return 1;
		}

	}
	return 0;
}

bool CmdHandler::counting() const
{
	return count_active;
}

/* count: count message in m_messageCounts */
void CmdHandler::count(const char *nick, const char *message)
{
	std::string msg = message;
	std::transform(msg.begin(), msg.end(), msg.begin(), tolower);

	/* if nick is not found */
	if (std::find(counted_users.begin(), counted_users.end(), nick)
			== counted_users.end()) {
		counted_users.push_back(nick);
		if (message_counts.find(msg) == message_counts.end())
			message_counts.insert({ msg, 1 });
		else
			message_counts.find(msg)->second++;
	}
}

/* process_default: run a default command */
void CmdHandler::process_default(char *out, struct command *c)
{
	if (P_ALSUB(c->privileges) || cooldowns.ready(c->argv[0])) {
		return_status = (this->*default_cmds[c->argv[0]])(out, c);
		cooldowns.set_used(c->argv[0]);
	} else {
		snprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
				c->nick, c->argv[0]);
		return_status = EXIT_FAILURE;
	}
}

/* process_custom: run a custom command */
void CmdHandler::process_custom(char *out, struct command *c)
{
	Json::Value *ccmd;

	ccmd = custom_cmds->getcom(c->argv[0]);
	if ((*ccmd)["active"].asBool() && (P_ALSUB(c->privileges) ||
				cooldowns.ready((*ccmd)["cmd"].asString()))) {
		/* set access time and increase uses */
		(*ccmd)["atime"] = (Json::Int64)time(nullptr);
		(*ccmd)["uses"] = (*ccmd)["uses"].asInt() + 1;
		snprintf(out, MAX_MSG, "%s", custom_cmds->format(ccmd, c->nick));
		cooldowns.set_used((*ccmd)["cmd"].asString());
		custom_cmds->write();
		return_status = EXIT_SUCCESS;
		return;
	}
	if (!(*ccmd)["active"].asBool()) {
		snprintf(out, MAX_MSG, "/w %s command is currently inactive: %s",
				c->nick, c->argv[0]);
		return_status = EXIT_FAILURE;
		return;
	}
	snprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
			c->nick, c->argv[0]);
	return_status = EXIT_FAILURE;
}

/* source: determine whether cmd is a default or custom command */
uint8_t CmdHandler::source(const char *cmd)
{
	Json::Value *c;

	if (default_cmds.find(cmd) != default_cmds.end())
		return DEFAULT;
	if (custom_cmds->active() && (c = custom_cmds->getcom(cmd)))
			return CUSTOM;
	return 0;
}

/* getrsn: print the rsn referred to by text into out */
int CmdHandler::getrsn(char *out, size_t len, const char *text,
		const char *nick, int username)
{
	const char *rsn;
	char *s;

	if (username) {
		if (!(rsn = stored_rsns.rsn(text))) {
			snprintf(out, len, "no RSN set for user %s", text);
			return 0;
		}
	} else {
		if (strcmp(text, ".") == 0) {
			if (!(rsn = stored_rsns.rsn(nick))) {
				snprintf(out, len, "no RSN set for user %s",
						nick);
				return 0;
			}
		} else {
			rsn = text;
		}
	}
	snprintf(out, len, "%s", rsn);
	while ((s = strchr(out, ' ')))
		*s = '_';
	return 1;
}

/* populateCmds: initialize pointers to all default commands */
void CmdHandler::add_commands()
{
	/* regular commands */
	default_cmds["8ball"] = &CmdHandler::eightball;
	default_cmds["about"] = &CmdHandler::about;
	default_cmds["active"] = &CmdHandler::active;
	default_cmds["age"] = &CmdHandler::age;
	default_cmds["calc"] = &CmdHandler::calc;
	default_cmds["cgrep"] = &CmdHandler::cgrep;
	default_cmds["cmdinfo"] = &CmdHandler::cmdinfo;
	default_cmds["cml"] = &CmdHandler::cml;
	default_cmds["commands"] = &CmdHandler::manual;
	default_cmds["duck"] = &CmdHandler::duck;
	default_cmds["ehp"] = &CmdHandler::ehp;
	default_cmds["fashiongen"] = &CmdHandler::fashiongen;
	default_cmds["ge"] = &CmdHandler::ge;
	default_cmds["help"] = &CmdHandler::man;
	default_cmds["level"] = &CmdHandler::level;
	default_cmds["lvl"] = &CmdHandler::level;
	default_cmds["man"] = &CmdHandler::man;
	default_cmds["manual"] = &CmdHandler::manual;
	default_cmds["rsn"] = &CmdHandler::rsn;
	default_cmds["submit"] = &CmdHandler::submit;
	default_cmds["twitter"] = &CmdHandler::twitter;
	default_cmds["uptime"] = &CmdHandler::uptime;
	default_cmds["xp"] = &CmdHandler::xp;
	default_cmds[swheel.cmd()] = &CmdHandler::wheel;

	/* moderator commands */
	default_cmds["ac"] = &CmdHandler::addcom;
	default_cmds["addcom"] = &CmdHandler::addcom;
	default_cmds["addrec"] = &CmdHandler::addrec;
	default_cmds["ar"] = &CmdHandler::addrec;
	default_cmds["count"] = &CmdHandler::count;
	default_cmds["dc"] = &CmdHandler::delcom;
	default_cmds["delcom"] = &CmdHandler::delcom;
	default_cmds["delrec"] = &CmdHandler::delrec;
	default_cmds["dr"] = &CmdHandler::delrec;
	default_cmds["ec"] = &CmdHandler::editcom;
	default_cmds["editcom"] = &CmdHandler::editcom;
	default_cmds["er"] = &CmdHandler::editrec;
	default_cmds["editrec"] = &CmdHandler::editrec;
	default_cmds["lsrec"] = &CmdHandler::showrec;
	default_cmds["mod"] = &CmdHandler::mod;
	default_cmds["permit"] = &CmdHandler::permit;
	default_cmds["setgiv"] = &CmdHandler::setgiv;
	default_cmds["setrec"] = &CmdHandler::setrec;
	default_cmds["showrec"] = &CmdHandler::showrec;
	default_cmds["sp"] = &CmdHandler::strawpoll;
	default_cmds["status"] = &CmdHandler::status;
	default_cmds["strawpoll"] = &CmdHandler::strawpoll;
	default_cmds["whitelist"] = &CmdHandler::whitelist;

	/* set all command cooldowns */
	for (auto &p : default_cmds)
		cooldowns.add(p.first);
	cooldowns.add(swheel.name());
}

/* populateHelp: fill m_help with manual page names */
void CmdHandler::populate_help()
{
	help[swheel.cmd()] = "selection-wheel";
	help["wheel"] = "selection-wheel";
	help["ac"] = "addcom";
	help["dc"] = "delcom";
	help["ec"] = "editcom";
	help["ar"] = "addrec";
	help["dr"] = "delrec";
	help["er"] = "editrec";
	help["lsrec"] = "showrec";
	help["lvl"] = "level";
	help["sp"] = "strawpoll";
	help["automated-responses"] = "automated-responses";
	help["responses"] = "automated-responses";
	help["custom"] = "custom-commands";
	help["custom-commands"] = "custom-commands";
	help["giveaway"] = "giveaways";
	help["giveaways"] = "giveaways";
	help["help"] = "man";
	help["moderation"] = "moderation";
	help["moderator"] = "moderation";
	help["recurring"] = "recurring-messages";
	help["recurring-messages"] = "recurring-messages";
	help["rsns"] = "rsn-management";
	help["rsn-management"] = "rsn-management";
	help["submessage"] = "subscriber-message";
	help["tweet"] = "tweet-reader";
	help["tweet-reader"] = "tweet-reader";
	help["twitter"] = "tweet-reader";
	help["twitch"] = "twitch-authorization";
	help["twitchauth"] = "twitch-authorization";
	help["twitch-autorization"] = "twitch-authorization";
	help["auth"] = "twitch-authorization";
	help["authorization"] = "twitch-authorization";
}
