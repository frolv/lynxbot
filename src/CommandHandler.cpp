#include <algorithm>
#include <ctime>
#include <cpr/cpr.h>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <utils.h>
#include <tw/oauth.h>
#include "CommandHandler.h"
#include "OptionParser.h"

#define DEFAULT 1
#define CUSTOM 2

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
		std::cerr << " Custom commands will be disabled "
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
		if (P_ALSUB(p) || m_cooldowns.ready((*ccmd)["cmd"].asString())) {
			output += (*ccmd)["response"].asString();
			m_cooldowns.setUsed((*ccmd)["cmd"].asString());
		} else {
			output += "/w " + nick + " command is on cooldown: "
				+ cmd;
		}
		break;
	default:
		output += "/w " + nick + " not a bot command: " + cmd;
		break;
	}

	return output;
}

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

/* wheel: select items from various categories */
std::string CommandHandler::wheel(struct cmdinfo *c)
{
	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() == 1)
		return m_wheel.name() + ": " + m_wheel.desc()
			+ " " + m_wheel.usage();
	if (argv.size() > 2 || (!m_wheel.valid(argv[1]) && argv[1] != "check"))
		return c->cmd + ": invalid syntax. " + m_wheel.usage();

	std::string output = "@" + c->nick + ", ";
	if (argv[1] == "check") {
		/* return the current selection */
		output += m_wheel.ready(c->nick)
			? "you are not currently assigned anything."
			: "you are currently assigned "
			+ m_wheel.selection(c->nick) + ".";
	} else if (!m_wheel.ready(c->nick)) {
		output += "you have already been assigned something!";
	} else {
		/* make a new selection */
		output += "your entertainment for tonight is "
			+ m_wheel.choose(c->nick, argv[1]) + ".";
	}

	return output;
}

/* strawpoll: create polls */
std::string CommandHandler::strawpoll(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return "";

	/* json values to hold created poll, poll options and http response */
	Json::Value poll, options(Json::arrayValue), response;
	std::string output = "[SPOLL] ";
	bool binary = false, multi = false, captcha = false;

	int opt;
	OptionParser op(c->fullCmd, "bcm");
	static struct OptionParser::option long_opts[] = {
		{ "binary", NO_ARG, 'b' },
		{ "captcha", NO_ARG, 'c' },
		{ "multi", NO_ARG, 'm' },
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'b':
			binary = true;
			break;
		case 'c':
			captcha = true;
			break;
		case 'm':
			multi = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() >= c->fullCmd.length())
		return c->cmd + ": not enough arguments given";
	const std::string req = c->fullCmd.substr(op.optind());
	std::vector<std::string> argv;
	utils::split(req, '|', argv);

	if (binary && argv.size() != 1)
		return c->cmd + ": cannot provide options for binary poll";
	if (!binary && argv.size() < 3)
		return c->cmd + ": poll must have a question and at least "
			"two answers";

	if (binary) {
		options.append("yes");
		options.append("no");
	} else {
		for (size_t i = 1; i < argv.size(); ++i)
			options.append(argv[i]);
	}

	/* populate the poll json */
	poll["title"] = argv[0];
	poll["options"] = options;
	poll["captcha"] = captcha;
	poll["multi"] = multi;

	/* format and post the poll */
	Json::FastWriter fw;
	const std::string content = fw.write(poll);
	cpr::Response resp = cpr::Post(cpr::Url("http://" + STRAWPOLL_HOST
		+ STRAWPOLL_API), cpr::Body(content), cpr::Header{
		{ "Connection", "close" },
		{ "Content-Type", "application/json" },
		{ "Content-Length", std::to_string(content.length()) } });

	Json::Reader reader;
	if (reader.parse(resp.text, response)) {
		if (!response.isMember("id")) {
			return c->cmd + ": poll could not be created";
		} else {
			m_activePoll = "http://" + STRAWPOLL_HOST + "/"
				+ response["id"].asString();
			output += "Poll created: " + m_activePoll;
		}
	} else {
		return c->cmd + ": error connecting to server";
	}

	return output;
}

/* uptime: check how long channel has been live */
std::string CommandHandler::uptime(struct cmdinfo *c)
{
	std::string out = "@" + c->nick + ", ";
	cpr::Response resp = cpr::Get(
		cpr::Url("https://decapi.me/twitch/uptime.php?channel="
			+ m_channel),
		cpr::Header{{ "Connection", "close" }});
	static const std::string channel =
		(char)toupper(m_channel[0]) + m_channel.substr(1);
	if (resp.text.substr(0, 7) == "Channel")
		return out + channel + " is not currently live.";
	else
		return out + channel + " has been live for " + resp.text + ".";
}

/* submit: submit a message to the streamer */
std::string CommandHandler::submit(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 7)
		return c->cmd + ": nothing to submit";

	std::string output = "@" + c->nick + ", ";
	std::string path = utils::configdir() + utils::config("submit");
	std::ofstream writer(path, std::ios::app);

	writer << c->nick << ": " << c->fullCmd.substr(7) << std::endl;
	writer.close();

	return output + "your topic has been submitted. Thank you.";
}

/* whitelist: exempt websites from moderation */
std::string CommandHandler::whitelist(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return "";

	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() > 2)
		return c->cmd + ": invalid syntax. Use \"$whitelist [SITE]\"";
	/* no args: show current whitelist */
	if (argv.size() == 1)
		return m_modp->getFormattedWhitelist();

	if (m_parsep->parse(argv[1])) {
		/* extract website and add to whitelist */
		std::string website = m_parsep->getLast()->domain;
		if (m_modp->whitelist(website))
			return "@" + c->nick + ", " + website
				+ " has been whitelisted.";
		else
			return "@" + c->nick + ", " + website
				+ " is already on the whitelist.";
	}

	return c->cmd + ": invalid URL: " + argv[1];
}

/* makecom: create or modify a custom command */
std::string CommandHandler::makecom(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return "";

	if (!m_customCmds->isActive())
		return "Custom commands are currently disabled.";

	bool editing = c->cmd == "editcom";
	std::string output = "@" + c->nick + ", ";
	time_t cooldown = editing ? -1 : 15;

	OptionParser op(c->fullCmd, "c:");
	int opt;
	static struct OptionParser::option long_opts[] = {
		{ "cooldown", REQ_ARG, 'c'},
		{ 0, 0, 0 }
	};

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'c':
			/* user provided a cooldown */
			try {
				cooldown = std::stoi(op.optarg());
				if (cooldown < 0)
					return c->cmd + ": cooldown cannot be "
						"negative";
			} catch (std::invalid_argument) {
				output = c->cmd  + ": invalid number -- ";
				return output + op.optarg();
			}
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	if (op.optind() >= c->fullCmd.length())
		return c->cmd + ": not enough arguments given";

	std::string args = c->fullCmd.substr(op.optind());
	std::string::size_type sp = args.find(' ');
	if (editing) {
		/* determine which parts are being changed */
		bool changedCd = cooldown != -1;
		bool changedResp = sp != std::string::npos;
		const std::string cmd = changedResp ? args.substr(0, sp) : args;
		std::string response = changedResp
			? args.substr(sp + 1) : "";
		/* don't allow reponse to activate a twitch command */
		if (response[0] == '/')
			response = " " + response;
		if (!m_customCmds->editCom(cmd, response, cooldown)) {
			return c->cmd + ": invalid command: $" + cmd;
		} else if (!changedCd && !changedResp) {
			output += "command $" + cmd + " was unchanged";
		} else {
			/* build an output string detailing changes */
			output += "command $" + cmd + " has been changed to ";
			if (changedResp)
				output += "\"" + response + "\""
					+ (changedCd ? ", with " : ".");
			if (changedCd) {
				/* reset cooldown in TimerManager */
				m_cooldowns.remove(cmd);
				m_cooldowns.add(cmd, cooldown);
				output += "a " + std::to_string(cooldown)
					+ "s cooldown.";
			}
		}
	} else {
		/* adding a new command */
		if (sp == std::string::npos) {
			return c->cmd + ": no response provided for command $"
				+ args;
		} else {
			/* first word is command, rest is response */
			std::string cmd = args.substr(0, sp);
			std::string response = args.substr(sp + 1);
			/* don't allow reponse to activate a twitch command */
			if (response[0] == '/')
				response = " " + response;
			if (!m_customCmds->addCom(cmd, response, cooldown))
				return c->cmd + ": invalid command name: $"
					+ cmd;
			else
				output += "command $" + cmd
					+ " has been added with a " +
					std::to_string(cooldown) + "s cooldown.";
		}
	}
	return output;
}

/* setrec: enable and disable recurring messages */
std::string CommandHandler::setrec(struct cmdinfo *c)
{
	if (!P_ALMOD(c->privileges))
		return "";

	std::string output = "@" + c->nick + ", ";
	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() != 2 || (argv[1] != "on" && argv[1] != "off"))
		return c->cmd + ": invalid syntax. Use \"setrec on|off\"";

	if (argv[1] == "on") {
		m_evtp->activateMessages();
		output += "recurring messages enabled.";
	} else {
		m_evtp->deactivateMessages();
		output += "recurring messages disabled.";
	}
	return output;
}

/* status: set channel status */
std::string CommandHandler::status(struct cmdinfo *c)
{
	int opt;
	OptionParser op(c->fullCmd, "a");
	std::string output, status;
	bool append = false;

	static struct OptionParser::option long_opts[] = {
		{ "append", NO_ARG, 'a' },
		{ 0, 0, 0 }
	};

	cpr::Response resp;
	cpr::Header head{{ "Accept", "application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + m_token }};
	Json::Reader reader;
	Json::Value response;

	if (!P_ALMOD(c->privileges))
		return "";

	while ((opt = op.getopt_long(long_opts)) != EOF) {
		switch (opt) {
		case 'a':
			append = true;
			break;
		case '?':
			return std::string(op.opterr());
		default:
			return "";
		}
	}

	output = "[STATUS] ";

	/* get the current status if no arg provided or if appending */
	if (op.optind() == c->fullCmd.length() || append) {
		resp = cpr::Get(cpr::Url(TWITCH_API + "/channels/" + m_channel),
				head);
		if (reader.parse(resp.text, response)) {
			if (response.isMember("error"))
				return c->cmd + ": "
					+ response["error"].asString();
			status = response["status"].asString();
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (append)
			return c->cmd + ": no text to append";
		return output + "Current status for " + m_channel + " is \""
			+ status + "\".";
	}

	if (append)
		status += " ";
	status += c->fullCmd.substr(op.optind());
	status = tw::pencode(status, " ");
	std::replace(status.begin(), status.end(), ' ', '+');
	std::string content = "channel[status]=" + status;

	resp = cpr::Put(cpr::Url(TWITCH_API + "/channels/"
				+ m_channel), cpr::Body(content), head);

	if (reader.parse(resp.text, response)) {
		if (response.isMember("error"))
			return c->cmd + ": " + response["error"].asString();
		return "[STATUS] Channel status changed to \""
			+ response["status"].asString() + "\".";
	}
	return c->cmd + ": something went wrong, try again";
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

/* getRsn: find the rsn referred to by text */
std::string CommandHandler::getRSN(const std::string &text,
	const std::string &nick, std::string &err, bool username)
{
	std::string rsn;
	if (username) {
		rsn = m_rsns.getRSN(text);
		if (rsn.empty())
			err = "No RSN set for user " + text + ".";
	} else {
		if (text == ".") {
			if ((rsn = m_rsns.getRSN(nick)).empty())
				err = "No RSN set for user " + nick + ". "
					"Use \"$rsn set RSN\" to set one.";
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
	m_defaultCmds["ehp"] = &CommandHandler::ehp;
	m_defaultCmds["level"] = &CommandHandler::level;
	m_defaultCmds["lvl"] = &CommandHandler::level;
	m_defaultCmds["ge"] = &CommandHandler::ge;
	m_defaultCmds["calc"] = &CommandHandler::calc;
	m_defaultCmds["cml"] = &CommandHandler::cml;
	m_defaultCmds["8ball"] = &CommandHandler::eightball;
	m_defaultCmds["active"] = &CommandHandler::active;
	m_defaultCmds["uptime"] = &CommandHandler::uptime;
	m_defaultCmds["rsn"] = &CommandHandler::rsn;
	m_defaultCmds["manual"] = &CommandHandler::manual;
	m_defaultCmds["commands"] = &CommandHandler::manual;
	m_defaultCmds["help"] = &CommandHandler::help;
	m_defaultCmds["about"] = &CommandHandler::about;
	m_defaultCmds["submit"] = &CommandHandler::submit;
	m_defaultCmds["duck"] = &CommandHandler::duck;
	m_defaultCmds[m_wheel.cmd()] = &CommandHandler::wheel;

	/* moderator commands */
	m_defaultCmds["sp"] = &CommandHandler::strawpoll;
	m_defaultCmds["strawpoll"] = &CommandHandler::strawpoll;
	m_defaultCmds["count"] = &CommandHandler::count;
	m_defaultCmds["whitelist"] = &CommandHandler::whitelist;
	m_defaultCmds["permit"] = &CommandHandler::permit;
	m_defaultCmds["addcom"] = &CommandHandler::makecom;
	m_defaultCmds["editcom"] = &CommandHandler::makecom;
	m_defaultCmds["delcom"] = &CommandHandler::delcom;
	m_defaultCmds["addrec"] = &CommandHandler::addrec;
	m_defaultCmds["delrec"] = &CommandHandler::delrec;
	m_defaultCmds["showrec"] = &CommandHandler::showrec;
	m_defaultCmds["setrec"] = &CommandHandler::setrec;
	m_defaultCmds["setgiv"] = &CommandHandler::setgiv;
	m_defaultCmds["status"] = &CommandHandler::status;
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
