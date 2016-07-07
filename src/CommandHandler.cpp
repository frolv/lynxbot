#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <regex>
#include <utils.h>
#include "CommandHandler.h"
#include "OptionParser.h"

CommandHandler::CommandHandler(const std::string &name,
		const std::string &channel, const std::string &token,
		Moderator *mod, URLParser *urlp, EventManager *evtp,
		Giveaway *givp, ConfigReader *cfgr)
	: m_name(name), m_channel(channel), m_token(token), m_modp(mod),
	m_parsep(urlp), m_customCmds(NULL), m_evtp(evtp), m_givp(givp),
	m_cfgr(cfgr), m_counting(false), m_gen(m_rd())
{
	populateCmds();

	m_customCmds = new CustomCommandHandler(&m_defaultCmds, &m_cooldowns,
			m_wheel.cmd());
	if (!m_customCmds->isActive()) {
		std::cerr << "Custom commands will be disabled "
			"for this session." << std::endl;
		std::cin.get();
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
		std::cin.get();
	}

	/* set all command cooldowns */
	for (auto &p : m_defaultCmds)
		m_cooldowns.add(p.first);
	m_cooldowns.add(m_wheel.name(), 10);

	/* read extra 8ball responses */
	utils::split(m_cfgr->get("extra8ballresponses"), '\n', m_eightball);

	populateHelp();
}

CommandHandler::~CommandHandler()
{
	delete m_customCmds;
}

/* processCommand: run message as a bot command and return output */
std::string CommandHandler::processCommand(const std::string &nick,
	const std::string &fullCmd, perm_t p)
{
	std::string output = "";

	/* the command is the first part of the string up to the first space */
	std::string cmd = fullCmd.substr(0, fullCmd.find(' '));

	struct cmdinfo c;
	c.nick = nick;
	c.cmd = cmd;
	c.fullCmd = fullCmd;
	c.privileges = p;

	/* custom command */
	Json::Value *ccmd;

	switch (source(cmd)) {
	case DEFAULT:
		if (P_ALSUB(p) || m_cooldowns.ready(cmd)) {
			output += (this->*m_defaultCmds[cmd])(&c);
			m_cooldowns.setUsed(cmd);
		} else {
			output += "/w " + nick + " command is on cooldown: "
				+ cmd;
		}
		break;
	case CUSTOM:
		ccmd = m_customCmds->getCom(cmd);
		if ((*ccmd)["active"].asBool() &&
				(P_ALSUB(p) ||
				m_cooldowns.ready((*ccmd)["cmd"].asString()))) {
			output += (*ccmd)["response"].asString();
			(*ccmd)["atime"] = (Json::Int64)time(nullptr);
			(*ccmd)["uses"] = (*ccmd)["uses"].asInt() + 1;
			m_cooldowns.setUsed((*ccmd)["cmd"].asString());
			m_customCmds->write();
		} else {
			if (!(*ccmd)["active"].asBool())
				output += "/w " + nick + " command is "
					"currently inactive: " + cmd;
			else
				output += "/w " + nick + " command is on "
					"cooldown: " + cmd;
		}
		break;
	default:
		output += "/w " + nick + " not a bot command: " + cmd;
		break;
	}

	return output;
}

/* processResponse: test a message against auto response regexes */
std::string CommandHandler::processResponse(const std::string &message)
{
	if (!m_responding)
		return "";

	/* test the message against all response regexes */
	for (auto &val : m_responses["responses"]) {
		std::string name = '_' + val["name"].asString();
		std::string regex = val["regex"].asString();

		std::regex responseRegex(regex,
				std::regex_constants::ECMAScript
				| std::regex_constants::icase);
		std::smatch match;
		if (std::regex_search(message.begin(), message.end(), match,
				responseRegex) && m_cooldowns.ready(name)) {
			m_cooldowns.setUsed(name);
			return val["response"].asString();
		}

	}
	/* no match */
	return "";
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

/* source: determine whether cmd is a default or custom command */
uint8_t CommandHandler::source(const std::string &cmd)
{
	if (m_defaultCmds.find(cmd) != m_defaultCmds.end())
		return DEFAULT;
	if (m_customCmds->isActive()) {
		Json::Value *c;
		if (!(c = m_customCmds->getCom(cmd))->empty())
			return CUSTOM;
	}
	return 0;
}

/* getRSN: find the rsn referred to by text */
std::string CommandHandler::getRSN(const std::string &text,
	const std::string &nick, std::string &err, bool username)
{
	std::string rsn;
	if (username) {
		rsn = m_rsns.getRSN(text);
		if (rsn.empty())
			err = "no RSN set for user " + text;
	} else {
		if (text == ".") {
			if ((rsn = m_rsns.getRSN(nick)).empty())
				err = "no RSN set for user " + nick;
		} else {
			rsn = text;
		}
	}
	return rsn;
}

/* populateCmds: initialize pointers to all default commands */
void CommandHandler::populateCmds()
{
	/* regular commands */
	m_defaultCmds["8ball"] = &CommandHandler::eightball;
	m_defaultCmds["about"] = &CommandHandler::about;
	m_defaultCmds["active"] = &CommandHandler::active;
	m_defaultCmds["calc"] = &CommandHandler::calc;
	m_defaultCmds["cgrep"] = &CommandHandler::cgrep;
	m_defaultCmds["cmdinfo"] = &CommandHandler::cmdinf;
	m_defaultCmds["cml"] = &CommandHandler::cml;
	m_defaultCmds["commands"] = &CommandHandler::manual;
	m_defaultCmds["duck"] = &CommandHandler::duck;
	m_defaultCmds["ehp"] = &CommandHandler::ehp;
	m_defaultCmds["ge"] = &CommandHandler::ge;
	m_defaultCmds["help"] = &CommandHandler::help;
	m_defaultCmds["level"] = &CommandHandler::level;
	m_defaultCmds["lvl"] = &CommandHandler::level;
	m_defaultCmds["manual"] = &CommandHandler::manual;
	m_defaultCmds["rsn"] = &CommandHandler::rsn;
	m_defaultCmds["submit"] = &CommandHandler::submit;
	m_defaultCmds["uptime"] = &CommandHandler::uptime;
	m_defaultCmds["xp"] = &CommandHandler::xp;
	m_defaultCmds[m_wheel.cmd()] = &CommandHandler::wheel;

	/* moderator commands */
	m_defaultCmds["addcom"] = &CommandHandler::addcom;
	m_defaultCmds["addrec"] = &CommandHandler::addrec;
	m_defaultCmds["count"] = &CommandHandler::count;
	m_defaultCmds["delcom"] = &CommandHandler::delcom;
	m_defaultCmds["delrec"] = &CommandHandler::delrec;
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
	m_help["lvl"] = "level";
	m_help["sp"] = "strawpoll";
	m_help["automated-responses"] = "automated-responses";
	m_help["responses"] = "automated-responses";
	m_help["custom"] = "custom-commands";
	m_help["custom-commands"] = "custom-commands";
	m_help["giveaway"] = "giveaways";
	m_help["giveaways"] = "giveaways";
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
