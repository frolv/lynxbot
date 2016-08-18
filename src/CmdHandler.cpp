#include <algorithm>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <regex>
#include <utils.h>
#include "cmdparse.h"
#include "CmdHandler.h"
#include "lynxbot.h"

CmdHandler::CmdHandler(const char *name, const char *channel,
		const char *token, Moderator *mod, URLParser *urlp,
		EventManager *evtp, Giveaway *givp, ConfigReader *cfgr,
		tw::Authenticator *auth)
	: m_name(name), m_channel(channel), m_token(token), m_modp(mod),
	m_parsep(urlp), m_customCmds(NULL), m_evtp(evtp), m_givp(givp),
	m_cfgr(cfgr), m_auth(auth), m_counting(false), m_gen(m_rd()),
	m_status(EXIT_SUCCESS)
{
	populate_cmd();
	m_poll[0] = '\0';

	m_customCmds = new CustomHandler(&m_defaultCmds, &m_cooldowns,
			m_wheel.cmd(), name, channel);
	if (!m_customCmds->isActive()) {
		fprintf(stderr, "Custom commands will be "
				"disabled for this session\n");
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
		fprintf(stderr, "Failed to read responses.json. "
				"Responses disabled for this session\n");
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
		fprintf(stderr, "Could not read fashion.json\n");

	populate_help();
}

CmdHandler::~CmdHandler()
{
	delete m_customCmds;
}

/* process_cmd: run message as a bot command and store output in out */
void CmdHandler::process_cmd(char *out, char *nick, char *cmdstr, perm_t p)
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
int CmdHandler::process_resp(char *out, char *msg, char *nick)
{
	const std::string message(msg);
	std::regex respreg;
	std::smatch match;
	std::string name, regex;

	*out = '\0';
	if (!m_responding)
		return 0;

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
			return 1;
		}

	}
	return 0;
}

bool CmdHandler::counting() const
{
	return m_counting;
}

/* count: count message in m_messageCounts */
void CmdHandler::count(const char *nick, const char *message)
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
void CmdHandler::process_default(char *out, struct command *c)
{
	if (P_ALSUB(c->privileges) || m_cooldowns.ready(c->argv[0])) {
		m_status = (this->*m_defaultCmds[c->argv[0]])(out, c);
		m_cooldowns.setUsed(c->argv[0]);
	} else {
		_sprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
				c->nick, c->argv[0]);
		m_status = EXIT_FAILURE;
	}
}

/* process_custom: run a custom command */
void CmdHandler::process_custom(char *out, struct command *c)
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
		m_status = EXIT_SUCCESS;
		return;
	}
	if (!(*ccmd)["active"].asBool()) {
		_sprintf(out, MAX_MSG, "/w %s command is currently inactive: %s",
				c->nick, c->argv[0]);
		m_status = EXIT_FAILURE;
		return;
	}
	_sprintf(out, MAX_MSG, "/w %s command is on cooldown: %s",
			c->nick, c->argv[0]);
	m_status = EXIT_FAILURE;
}

/* source: determine whether cmd is a default or custom command */
uint8_t CmdHandler::source(const char *cmd)
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
int CmdHandler::getrsn(char *out, size_t len, const char *text,
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
void CmdHandler::populate_cmd()
{
	/* regular commands */
	m_defaultCmds["8ball"] = &CmdHandler::eightball;
	m_defaultCmds["about"] = &CmdHandler::about;
	m_defaultCmds["active"] = &CmdHandler::active;
	m_defaultCmds["age"] = &CmdHandler::age;
	m_defaultCmds["calc"] = &CmdHandler::calc;
	m_defaultCmds["cgrep"] = &CmdHandler::cgrep;
	m_defaultCmds["cmdinfo"] = &CmdHandler::cmdinfo;
	m_defaultCmds["cml"] = &CmdHandler::cml;
	m_defaultCmds["commands"] = &CmdHandler::manual;
	m_defaultCmds["duck"] = &CmdHandler::duck;
	m_defaultCmds["ehp"] = &CmdHandler::ehp;
	m_defaultCmds["fashiongen"] = &CmdHandler::fashiongen;
	m_defaultCmds["ge"] = &CmdHandler::ge;
	m_defaultCmds["help"] = &CmdHandler::man;
	m_defaultCmds["level"] = &CmdHandler::level;
	m_defaultCmds["lvl"] = &CmdHandler::level;
	m_defaultCmds["man"] = &CmdHandler::man;
	m_defaultCmds["manual"] = &CmdHandler::manual;
	m_defaultCmds["rsn"] = &CmdHandler::rsn;
	m_defaultCmds["submit"] = &CmdHandler::submit;
	m_defaultCmds["twitter"] = &CmdHandler::twitter;
	m_defaultCmds["uptime"] = &CmdHandler::uptime;
	m_defaultCmds["xp"] = &CmdHandler::xp;
	m_defaultCmds[m_wheel.cmd()] = &CmdHandler::wheel;

	/* moderator commands */
	m_defaultCmds["ac"] = &CmdHandler::addcom;
	m_defaultCmds["addcom"] = &CmdHandler::addcom;
	m_defaultCmds["addrec"] = &CmdHandler::addrec;
	m_defaultCmds["ar"] = &CmdHandler::addrec;
	m_defaultCmds["count"] = &CmdHandler::count;
	m_defaultCmds["dc"] = &CmdHandler::delcom;
	m_defaultCmds["delcom"] = &CmdHandler::delcom;
	m_defaultCmds["delrec"] = &CmdHandler::delrec;
	m_defaultCmds["dr"] = &CmdHandler::delrec;
	m_defaultCmds["ec"] = &CmdHandler::editcom;
	m_defaultCmds["editcom"] = &CmdHandler::editcom;
	m_defaultCmds["er"] = &CmdHandler::editrec;
	m_defaultCmds["editrec"] = &CmdHandler::editrec;
	m_defaultCmds["mod"] = &CmdHandler::mod;
	m_defaultCmds["permit"] = &CmdHandler::permit;
	m_defaultCmds["setgiv"] = &CmdHandler::setgiv;
	m_defaultCmds["setrec"] = &CmdHandler::setrec;
	m_defaultCmds["showrec"] = &CmdHandler::showrec;
	m_defaultCmds["sp"] = &CmdHandler::strawpoll;
	m_defaultCmds["status"] = &CmdHandler::status;
	m_defaultCmds["strawpoll"] = &CmdHandler::strawpoll;
	m_defaultCmds["whitelist"] = &CmdHandler::whitelist;
}

/* populateHelp: fill m_help with manual page names */
void CmdHandler::populate_help()
{
	m_help[m_wheel.cmd()] = "selection-wheel";
	m_help["wheel"] = "selection-wheel";
	m_help["ac"] = "addcom";
	m_help["dc"] = "delcom";
	m_help["ec"] = "editcom";
	m_help["ar"] = "addrec";
	m_help["dr"] = "delrec";
	m_help["er"] = "editrec";
	m_help["lvl"] = "level";
	m_help["sp"] = "strawpoll";
	m_help["automated-responses"] = "automated-responses";
	m_help["responses"] = "automated-responses";
	m_help["custom"] = "custom-commands";
	m_help["custom-commands"] = "custom-commands";
	m_help["giveaway"] = "giveaways";
	m_help["giveaways"] = "giveaways";
	m_help["help"] = "man";
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
