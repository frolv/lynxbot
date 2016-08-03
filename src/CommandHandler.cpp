#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <regex>
#include <utils.h>
#include "cmdparse.h"
#include "CommandHandler.h"
#include "lynxbot.h"
#include "OptionParser.h"

CommandHandler::CommandHandler(const char *name, const char *channel,
		const char *token, Moderator *mod, URLParser *urlp,
		EventManager *evtp, Giveaway *givp, ConfigReader *cfgr,
		tw::Authenticator *auth)
	: m_name(name), m_channel(channel), m_token(token), m_modp(mod),
	m_parsep(urlp), m_customCmds(NULL), m_evtp(evtp), m_givp(givp),
	m_cfgr(cfgr), m_auth(auth), m_counting(false), m_gen(m_rd())
{
	populateCmds();
	m_poll[0] = '\0';

	m_customCmds = new CustomCommandHandler(&m_defaultCmds, &m_cooldowns,
			m_wheel.cmd(), name, channel);
	if (!m_customCmds->isActive()) {
		std::cerr << "Custom commands will be disabled "
			"for this session." << std::endl;
		WAIT_INPUT();
	}

	/* read all responses from file */
	m_responding = utils::readJSON("responses.json", m_responses);
	if (m_responding) {
		/* add response cooldowns to TimerManager */
		for (auto &val : m_responses["responses"])
			m_cooldowns.add('_' + val["name"].asString(),
				val["cooldown"].asInt());
	} else {
		std::cerr << "Failed to read responses.json. "
			"Responses disabled for this session.";
		WAIT_INPUT();
	}

	/* set all command cooldowns */
	for (auto &p : m_defaultCmds)
		m_cooldowns.add(p.first);
	m_cooldowns.add(m_wheel.name());

	/* read extra 8ball responses */
	utils::split(m_cfgr->get("extra8ballresponses"), '\n', m_eightball);

	/* read fashion.json */
	if (!utils::readJSON("fashion.json", m_fashion))
		std::cerr << "Could not read fashion.json" << std::endl;

	populateHelp();
}

CommandHandler::~CommandHandler()
{
	delete m_customCmds;
}

/* process_cmd: run message as a bot command and store output in out */
void CommandHandler::process_cmd(char *out, char *nick, char *cmdstr, perm_t p)
{
	struct command c;

	c.nick = nick;
	c.privileges = p;
	if (!parse_cmd(cmdstr, &c)) {
		_sprintf(out, MAX_MSG, "%s", cmderr());
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
		_sprintf(out, MAX_MSG, "/w %s not a bot command: %s",
				nick, c.argv[0]);
		break;
	}
	free_cmd(&c);
}

/* process_resp: test a message against auto response regexes */
void CommandHandler::process_resp(char *out, char *msg, char *nick)
{
	const std::string message(msg);
	std::regex respreg;
	std::smatch match;
	std::string name, regex;

	*out = '\0';
	if (!m_responding)
		return;

	/* test the message against all response regexes */
	for (auto &val : m_responses["responses"]) {
		name = '_' + val["name"].asString();
		regex = val["regex"].asString();

		respreg = std::regex(regex, std::regex::icase);
		if (std::regex_search(message.begin(), message.end(), match,
				respreg) && m_cooldowns.ready(name)) {
			m_cooldowns.setUsed(name);
			_sprintf(out, MAX_MSG, "@%s, %s", nick,
					val["response"].asCString());
			return;
		}

	}
}

bool CommandHandler::isCounting() const
{
	return m_counting;
}

/* count: count message in m_messageCounts */
void CommandHandler::count(const std::string &nick, const std::string &message)
{
	std::string msg = message;
	std::transform(msg.begin(), msg.end(), msg.begin(), tolower);

	/* if nick is not found */
	if (std::find(m_usersCounted.begin(), m_usersCounted.end(), nick)
			== m_usersCounted.end()) {
		m_usersCounted.push_back(nick);
		if (m_messageCounts.find(msg) == m_messageCounts.end())
			m_messageCounts.insert({ msg, 1 });
		else
			m_messageCounts.find(msg)->second++;
	}
}

/* process_default: run a default command */
void CommandHandler::process_default(char *out, struct command *c)
{
	/* while commands are converted to new format */
	std::string tmp;

	if (P_ALSUB(c->privileges) || m_cooldowns.ready(c->argv[0])) {
		if (!(tmp = (this->*m_defaultCmds[c->argv[0]])(out, c)).empty()) {
			_sprintf(out, MAX_MSG, "%s", tmp.c_str());
			m_cooldowns.setUsed(c->argv[0]);
		}
		return;
	}
	_sprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
			c->nick, c->argv[0]);
}

/* process_custom: run a custom command */
void CommandHandler::process_custom(char *out, struct command *c)
{
	Json::Value *ccmd;

	ccmd = m_customCmds->getcom(c->argv[0]);
	if ((*ccmd)["active"].asBool() && (P_ALSUB(c->privileges) ||
				m_cooldowns.ready((*ccmd)["cmd"].asString()))) {
		/* set access time and increase uses */
		(*ccmd)["atime"] = (Json::Int64)time(nullptr);
		(*ccmd)["uses"] = (*ccmd)["uses"].asInt() + 1;
		_sprintf(out, MAX_MSG, "%s",
				m_customCmds->format(ccmd, c->nick).c_str());
		m_cooldowns.setUsed((*ccmd)["cmd"].asString());
		m_customCmds->write();
		return;
	}
	if (!(*ccmd)["active"].asBool()) {
		_sprintf(out, MAX_MSG, "/w %s command is currently inactive: %s",
				c->nick, c->argv[0]);
		return;
	}
	_sprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
			c->nick, c->argv[0]);
}

/* source: determine whether cmd is a default or custom command */
uint8_t CommandHandler::source(const std::string &cmd)
{
	if (m_defaultCmds.find(cmd) != m_defaultCmds.end())
		return DEFAULT;
	if (m_customCmds->isActive()) {
		Json::Value *c;
		if (!(c = m_customCmds->getcom(cmd))->empty())
			return CUSTOM;
	}
	return 0;
}

/* getrsn: print the rsn referred to by text into out */
int CommandHandler::getrsn(char *out, size_t len, const char *text,
		const char *nick, int username)
{
	const char *rsn;
	char *s;

	if (username) {
		if (!(rsn = m_rsns.rsn(text))) {
			_sprintf(out, len, "no RSN set for user %s", text);
			return 0;
		}
	} else {
		if (strcmp(text, ".") == 0) {
			if (!(rsn = m_rsns.rsn(nick))) {
				_sprintf(out, len, "no RSN set for user %s",
						nick);
				return 0;
			}
		} else {
			rsn = text;
		}
	}
	_sprintf(out, len, "%s", rsn);
	while ((s = strchr(out, ' ')))
		*s = '_';
	return 1;
}

/* populateCmds: initialize pointers to all default commands */
void CommandHandler::populateCmds()
{
	/* regular commands */
	m_defaultCmds["8ball"] = &CommandHandler::eightball;
	m_defaultCmds["about"] = &CommandHandler::about;
	m_defaultCmds["active"] = &CommandHandler::active;
	m_defaultCmds["age"] = &CommandHandler::age;
	m_defaultCmds["calc"] = &CommandHandler::calc;
	m_defaultCmds["cgrep"] = &CommandHandler::cgrep;
	m_defaultCmds["cmdinfo"] = &CommandHandler::cmdinfo;
	m_defaultCmds["cml"] = &CommandHandler::cml;
	m_defaultCmds["commands"] = &CommandHandler::manual;
	m_defaultCmds["duck"] = &CommandHandler::duck;
	m_defaultCmds["ehp"] = &CommandHandler::ehp;
	m_defaultCmds["fashiongen"] = &CommandHandler::fashiongen;
	m_defaultCmds["ge"] = &CommandHandler::ge;
	m_defaultCmds["help"] = &CommandHandler::man;
	m_defaultCmds["level"] = &CommandHandler::level;
	m_defaultCmds["lvl"] = &CommandHandler::level;
	m_defaultCmds["man"] = &CommandHandler::man;
	m_defaultCmds["manual"] = &CommandHandler::manual;
	m_defaultCmds["rsn"] = &CommandHandler::rsn;
	m_defaultCmds["submit"] = &CommandHandler::submit;
	m_defaultCmds["twitter"] = &CommandHandler::twitter;
	m_defaultCmds["uptime"] = &CommandHandler::uptime;
	m_defaultCmds["xp"] = &CommandHandler::xp;
	m_defaultCmds[m_wheel.cmd()] = &CommandHandler::wheel;

	/* moderator commands */
	m_defaultCmds["ac"] = &CommandHandler::addcom;
	m_defaultCmds["addcom"] = &CommandHandler::addcom;
	m_defaultCmds["addrec"] = &CommandHandler::addrec;
	m_defaultCmds["ar"] = &CommandHandler::addrec;
	m_defaultCmds["count"] = &CommandHandler::count;
	m_defaultCmds["dc"] = &CommandHandler::delcom;
	m_defaultCmds["delcom"] = &CommandHandler::delcom;
	m_defaultCmds["delrec"] = &CommandHandler::delrec;
	m_defaultCmds["dr"] = &CommandHandler::delrec;
	m_defaultCmds["ec"] = &CommandHandler::editcom;
	m_defaultCmds["editcom"] = &CommandHandler::editcom;
	m_defaultCmds["permit"] = &CommandHandler::permit;
	m_defaultCmds["setgiv"] = &CommandHandler::setgiv;
	m_defaultCmds["setrec"] = &CommandHandler::setrec;
	m_defaultCmds["showrec"] = &CommandHandler::showrec;
	m_defaultCmds["sp"] = &CommandHandler::strawpoll;
	m_defaultCmds["status"] = &CommandHandler::status;
	m_defaultCmds["strawpoll"] = &CommandHandler::strawpoll;
	m_defaultCmds["whitelist"] = &CommandHandler::whitelist;
}

/* populateHelp: fill m_help with manual page names */
void CommandHandler::populateHelp()
{
	m_help[m_wheel.cmd()] = "selection-wheel";
	m_help["wheel"] = "selection-wheel";
	m_help["ac"] = "addcom";
	m_help["dc"] = "delcom";
	m_help["ec"] = "editcom";
	m_help["ar"] = "addrec";
	m_help["dr"] = "delrec";
	m_help["lvl"] = "level";
	m_help["sp"] = "strawpoll";
	m_help["automated-responses"] = "automated-responses";
	m_help["responses"] = "automated-responses";
	m_help["custom"] = "custom-commands";
	m_help["custom-commands"] = "custom-commands";
	m_help["giveaway"] = "giveaways";
	m_help["giveaways"] = "giveaways";
	m_help["help"] = "man";
	m_help["mod"] = "moderation";
	m_help["moderation"] = "moderation";
	m_help["moderator"] = "moderation";
	m_help["recurring"] = "recurring-messages";
	m_help["recurring-messages"] = "recurring-messages";
	m_help["rsns"] = "rsn-management";
	m_help["rsn-management"] = "rsn-management";
	m_help["submessage"] = "subscriber-message";
	m_help["tweet"] = "tweet-reader";
	m_help["tweet-reader"] = "tweet-reader";
	m_help["twitter"] = "tweet-reader";
	m_help["twitch"] = "twitch-authorization";
	m_help["twitchauth"] = "twitch-authorization";
	m_help["twitch-autorization"] = "twitch-authorization";
	m_help["auth"] = "twitch-authorization";
	m_help["authorization"] = "twitch-authorization";
}
