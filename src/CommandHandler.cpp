#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <json/json.h>
#include <cpr/cpr.h>
#include <ExpressionParser.h>
#include "CommandHandler.h"
#include "OptionParser.h"
#include "SkillMap.h"
#include "utils.h"
#include "version.h"

CommandHandler::CommandHandler(const std::string &name,
		const std::string &channel, Moderator *mod,
		URLParser *urlp, EventManager *evtp)
	: m_name(name), m_channel(channel), m_modp(mod), m_parsep(urlp),
	m_customCmds(NULL), m_evtp(evtp), m_counting(false), m_gen(m_rd())
{
	/* initializing pointers to all default commands */
	m_defaultCmds["ehp"] = &CommandHandler::ehpFunc;
	m_defaultCmds["level"] = &CommandHandler::levelFunc;
	m_defaultCmds["lvl"] = &CommandHandler::levelFunc;
	m_defaultCmds["ge"] = &CommandHandler::geFunc;
	m_defaultCmds["calc"] = &CommandHandler::calcFunc;
	m_defaultCmds["cml"] = &CommandHandler::cmlFunc;
	m_defaultCmds["8ball"] = &CommandHandler::eightballFunc;
	m_defaultCmds["sp"] = &CommandHandler::strawpollFunc;
	m_defaultCmds["active"] = &CommandHandler::activeFunc;
	m_defaultCmds["count"] = &CommandHandler::countFunc;
	m_defaultCmds["uptime"] = &CommandHandler::uptimeFunc;
	m_defaultCmds["whitelist"] = &CommandHandler::whitelistFunc;
	m_defaultCmds["permit"] = &CommandHandler::permitFunc;
	m_defaultCmds["commands"] = &CommandHandler::commandsFunc;
	m_defaultCmds["about"] = &CommandHandler::aboutFunc;
	m_defaultCmds["addcom"] = &CommandHandler::makecomFunc;
	m_defaultCmds["editcom"] = &CommandHandler::makecomFunc;
	m_defaultCmds["delcom"] = &CommandHandler::delcomFunc;
	m_defaultCmds["addrec"] = &CommandHandler::addrecFunc;
	m_defaultCmds["delrec"] = &CommandHandler::delrecFunc;
	m_defaultCmds["listrec"] = &CommandHandler::listrecFunc;

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
	std::string path = utils::configdir() + utils::config("8ball");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		std::cerr << "could not read " + path << std::endl;
		return;
	}
	std::string line;
	while (std::getline(reader, line))
		m_eightballResponses.push_back(line);
	reader.close();
}

CommandHandler::~CommandHandler()
{
	delete m_customCmds;
}

std::string CommandHandler::processCommand(const std::string &nick,
	const std::string &fullCmd, bool privileges)
{
	std::string output = "";
	struct cmdinfo c;
	c.nick = nick;
	c.fullCmd = fullCmd;
	c.privileges = privileges;

	/* the command is the first part of the string up to the first space */
	std::string cmd = fullCmd.substr(0, fullCmd.find(' '));

	if (m_defaultCmds.find(cmd) != m_defaultCmds.end()
		&& (privileges || m_cooldowns.ready(cmd))) {
		output += (this->*m_defaultCmds[cmd])(&c);
		m_cooldowns.setUsed(cmd);
	} else if (m_wheel.isActive() && cmd == m_wheel.cmd()
		&& (privileges || m_cooldowns.ready(m_wheel.name()))) {
		output += wheelFunc(&c);
		m_cooldowns.setUsed(m_wheel.name());
	} else if (m_customCmds->isActive()) {
		/* check for custom command */
		Json::Value *customCmd = m_customCmds->getCom(cmd);
		if (!customCmd->empty() && (privileges || m_cooldowns.ready(
					(*customCmd)["cmd"].asString()))) {
			output += (*customCmd)["response"].asString();
			m_cooldowns.setUsed((*customCmd)["cmd"].asString());
		} else {
			std::cerr << "Invalid command or is on cooldown: "
				<< cmd << std::endl << std::endl;
		}
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
			std::regex_constants::ECMAScript | std::regex_constants::icase);
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
	if (std::find(m_usersCounted.begin(), m_usersCounted.end(), nick) ==
		m_usersCounted.end()) {
		m_usersCounted.push_back(nick);
		if (m_messageCounts.find(msg) == m_messageCounts.end())
			m_messageCounts.insert({ msg, 1 });
		else
			m_messageCounts.find(msg)->second++;
	}
}

std::string CommandHandler::ehpFunc(struct cmdinfo *c)
{
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() == 2) {
		/* a username was provided */
		std::string rsn = tokens[1];
		std::replace(rsn.begin(), rsn.end(), '-', '_');
		cpr::Response resp = cpr::Get(cpr::Url("http://" + CML_HOST +
			CML_EHP_API + rsn), cpr::Header{{ "Connection", "close" }});
		return "[EHP] " + extractCMLData(resp.text);

	} else if (tokens.size() == 1) {
		return "[EHP] EHP stands for efficient hours played. You earn "
			"1 EHP whenever you gain a certain amount of experience "
			"in a skill, depending on your level. You can find XP "
			"rates here: http://crystalmathlabs.com/tracker/suppliescalc.php . "
			"Watch a video explaining EHP: https://www.youtube.com/watch?v=rhxHlO8mvpc";
	} else {
		return "[EHP] Invalid syntax. Use \"$ehp [RSN]\".";
	}
}

std::string CommandHandler::levelFunc(struct cmdinfo *c)
{
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() == 3) {
		if (skillMap.find(tokens[1]) == skillMap.end()
			&& skillNickMap.find(tokens[1]) == skillNickMap.end())
			return "[LVL] Invalid skill name.";

		uint8_t skillID = skillMap.find(tokens[1]) == skillMap.end()
			? skillNickMap.find(tokens[1])->second
			: skillMap.find(tokens[1])->second;

		std::string rsn = tokens[2];
		std::replace(rsn.begin(), rsn.end(), '-', '_');

		cpr::Response resp = cpr::Get(cpr::Url("http://" + RS_HOST
			+ RS_HS_API + rsn), cpr::Header{{ "Connection", "close" }});
		if (resp.text.find("404 - Page not found") != std::string::npos)
			return "[LVL] Player not found on hiscores.";

		/* skill nickname is displayed to save space */
		std::string nick = getSkillNick(skillID);
		std::transform(nick.begin(), nick.end(), nick.begin(), toupper);

		return "[" + nick + "] Name: " + rsn + ", "
			+ extractHSData(resp.text, skillID);
	} else {
		return "[LVL] Invalid syntax. Use \"$lvl SKILL RSN\".";
	}
}

std::string CommandHandler::geFunc(struct cmdinfo *c)
{
	if (!m_GEReader.active())
		return "";
	if (c->fullCmd.length() < 4)
		return "[GE] No item name provided.";

	std::string itemName = c->fullCmd.substr(3);
	std::replace(itemName.begin(), itemName.end(), '_', ' ');

	Json::Value item = m_GEReader.getItem(itemName);
	if (item.empty())
		return "[GE] Item not found: " + itemName + ".";

	cpr::Response resp = cpr::Get(cpr::Url("http://" + EXCHANGE_HOST
		+ EXCHANGE_API + item["id"].asString()),
		cpr::Header{{ "Connection", "close" }});
	return "[GE] " + item["name"].asString() + ": "
		+ extractGEData(resp.text) + " gp.";
}

std::string CommandHandler::calcFunc(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6)
		return "Invalid mathematical expression.";

	std::string expr = c->fullCmd.substr(5);
	/* remove all whitespace */
	expr.erase(std::remove_if(expr.begin(), expr.end(), isspace), expr.end());
	
	std::string result;
	try {
		ExpressionParser exprP(expr);
		exprP.tokenizeExpr();
		double res = exprP.eval();
		result += std::to_string(res);
	} catch (std::runtime_error &e) {
		result = e.what();
	}
	if (result == "inf" || result == "-nan(ind)") {
		result = "Error: division by 0.";
	}

	return "[CALC] " + result;
}

std::string CommandHandler::cmlFunc(struct cmdinfo *c)
{
	return "[CML] http://" + CML_HOST + " @" + c->nick;
}

std::string CommandHandler::wheelFunc(struct cmdinfo *c)
{
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() == 1)
		return m_wheel.name() + ": " + m_wheel.desc() + " " + m_wheel.usage();
	if (tokens.size() > 2 || (!m_wheel.valid(tokens[1]) && tokens[1] != "check"))
		return "Invalid syntax. " + m_wheel.usage();

	std::string output = "@" + c->nick + ", ";
	if (tokens[1] == "check") {
		/* return the current selection */
		output += m_wheel.ready(c->nick)
			? "you are not currently assigned anything."
			: "you are currently assigned " + m_wheel.selection(c->nick) + ".";
	} else if (!m_wheel.ready(c->nick)) {
		output += "you have already been assigned something!";
	} else {
		/* make a new selection */
		output += "your entertainment for tonight is "
			+ m_wheel.choose(c->nick, tokens[1]) + ".";
	}

	return output;
}

std::string CommandHandler::eightballFunc(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6 || c->fullCmd[c->fullCmd.length() - 1] != '?')
		return "[8 BALL] Ask me a question.";
	std::uniform_int_distribution<> dis(0, m_eightballResponses.size());
	return "[8 BALL] @" + c->nick + ", " + m_eightballResponses[dis(m_gen)] + ".";
}

std::string CommandHandler::strawpollFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	/* json values to hold created poll, poll options and http response */
	Json::Value poll, options(Json::arrayValue), response;
	std::string output = "[SPOLL] ";
	bool binary = false, multi = false, captcha = false;

	int16_t opt;
	OptionParser op(c->fullCmd, "bcm");
	while ((opt = op.getopt()) != EOF) {
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
			return output + "illegal option: " + (char)op.optopt();
		default:
			return "";
		}
	}

	if (op.optind() >= c->fullCmd.length())
		return output + "Not enough arguments given.";
	const std::string req = c->fullCmd.substr(op.optind());
	std::vector<std::string> tokens;
	utils::split(req, '|', tokens);

	if (binary && tokens.size() != 1)
		return output + "Cannot provide options for binary poll.";
	if (!binary && tokens.size() < 3)
		return output + "Poll must have a question and at least two answers.";

	if (binary) {
		options.append("yes");
		options.append("no");
	} else {
		for (size_t i = 1; i < tokens.size(); ++i)
			options.append(tokens[i]);
	}

	/* populate the poll json */
	poll["title"] = tokens[0];
	poll["options"] = options;
	poll["captcha"] = captcha;
	poll["multi"] = multi;

	/* format and post the poll */
	Json::FastWriter fw;
	const std::string content = fw.write(poll);
	cpr::Response resp = cpr::Post(cpr::Url("http://" + STRAWPOLL_HOST
		+ STRAWPOLL_API), cpr::Body(content), cpr::Header{
		{ "Connection", "close" }, { "Content-Type", "application/json" },
		{ "Content-Length", std::to_string(content.length()) } });

	Json::Reader reader;
	if (reader.parse(resp.text, response)) {
		if (!response.isMember("id")) {
			output += "Poll could not be created.";
		} else {
			m_activePoll = "http://" + STRAWPOLL_HOST + "/"
				+ response["id"].asString();
			output += "Poll created : " + m_activePoll;
		}
	} else {
		output += "Error connecting to server.";
	}

	return output;
}

std::string CommandHandler::activeFunc(struct cmdinfo *c)
{
	return "[ACTIVE] @" + c->nick + ", " + (m_activePoll.empty()
		? "no poll has been created." : m_activePoll);
}

std::string CommandHandler::commandsFunc(struct cmdinfo *c)
{
	return "[COMMANDS] @" + c->nick + ", " + SOURCE + "/wiki/Default-Commands";
}

std::string CommandHandler::aboutFunc(struct cmdinfo *c)
{
	return "[ABOUT] @" + c->nick + ", " + m_name + " is running "
		+ BOT_NAME + " v" + BOT_VERSION + ". Find out more at " + SOURCE;
}

std::string CommandHandler::countFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() != 2 || !(tokens[1] == "start" || tokens[1] == "stop"
		|| tokens[1] == "display"))
		return "Invalid syntax.";

	if (tokens[1] == "start") {
		/* begin a new count */
		if (m_counting)
			return "A count is already running. "
				"End it before starting a new one.";
		m_usersCounted.clear();
		m_messageCounts.clear();
		m_counting = true;
		return "Message counting has begun. Prepend your message with "
			"a + to have it counted.";
	} else if (tokens[1] == "stop") {
		/* end the current count */
		if (!m_counting)
			return "There is no active count.";
		m_counting = false;
		return "Count ended. Use \"$count display\" to view the results.";
	} else {
		/* display results from last count */
		if (m_counting)
			return "End the count before viewing the results.";
		if (m_messageCounts.empty()) {
			return "Nothing to display.";
		} else {
			typedef std::pair<std::string, uint16_t> mcount;
			/* add each result to a vector to be sorted */
			std::vector<mcount> pairs;
			for (auto itr = m_messageCounts.begin();
				itr != m_messageCounts.end(); ++itr)
				pairs.push_back(*itr);
			std::sort(pairs.begin(), pairs.end(),
				[=](mcount &a, mcount &b) { return a.second > b.second; });
			std::string results = "[RESULTS] ";
			/* only show top 10 results */
			size_t max = pairs.size() > 10 ? 10 : pairs.size();
			for (size_t i = 0; i < max; ++i) {
				mcount &pair = pairs[i];
				/* print rank and number of votes */
				results += std::to_string(i + 1) + ". "
					+ pair.first + " ("
					+ std::to_string(pair.second) + ")"
					+ (i == max - 1 ? "." : ", ");
			}
			return results;
		}
	}
}

std::string CommandHandler::uptimeFunc(struct cmdinfo *c)
{
	/* don't believe me just watch */
	std::string out = "@" + c->nick + ", ";
	cpr::Response resp = cpr::Get(
		cpr::Url("https://decapi.me/twitch/uptime.php?channel=" + m_channel),
		cpr::Header{{ "Connection", "close" }});
	static const std::string channel =
		(char)toupper(m_channel[0]) + m_channel.substr(1);
	std::cout << resp.text << std::endl;
	if (resp.text.substr(0, 7) == "Channel")
		return out + channel + " is not currently live.";
	else
		return out + channel + " has been live for " + resp.text + ".";
}

std::string CommandHandler::whitelistFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() > 2)
		return "Invalid syntax. Use \"$whitelist [SITE]\".";
	/* no args: show current whitelist */
	if (tokens.size() == 1)
		return m_modp->getFormattedWhitelist();

	if (m_parsep->parse(tokens[1])) {
		/* extract website and add to whitelist */
		std::string website = m_parsep->getLast()->domain;
		if (m_modp->whitelist(website))
			return "@" + c->nick + ", " + website
				+ " has been whitelisted.";
		else
			return "@" + c->nick + ", " + website
				+ " is already on the whitelist.";
	}

	return "@" + c->nick + ", invalid URL: " + tokens[1];
}

std::string CommandHandler::permitFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() != 2)
		return "Invalid syntax. Use \"$permit USER\".";

	m_modp->permit(tokens[1]);
	return tokens[1] + " has been granted permission to post a link.";
}

std::string CommandHandler::makecomFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	if (!m_customCmds->isActive())
		return "Custom commands are currently disabled.";

	bool editing = c->fullCmd.substr(0, 4) == "edit";
	bool success = true;
	std::string output = "@" + c->nick + ", ";
	time_t cooldown = editing ? -1 : 15;

	OptionParser op(c->fullCmd, "c:");
	int16_t opt;
	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
		case 'c':
			/* user provided a cooldown */
			try {
				cooldown = std::stoi(op.optarg());
				if (cooldown < 0)
					return output
						+ "cooldown cannot be negative.";
			} catch (std::invalid_argument) {
				output += "invalid number: " + op.optarg();
				success = false;
			}
			break;
		case '?':
			/* invalid option */
			if (op.optopt() == 'c')
				output += "no cooldown provided.";
			else
				return output + "illegal option: "
					+ (char)op.optopt();
			success = false;
			break;
		default:
			return "";
		}
	}

	if (success && op.optind() >= c->fullCmd.length()) {
		output += "not enough arguments given.";
		success = false;
	}
	if (!success)
		return output;

	std::string args = c->fullCmd.substr(op.optind());
	std::string::size_type sp = args.find(' ');
	if (editing) {
		/* determine which parts are being changed */
		bool changedCd = cooldown != -1;
		bool changedResp = sp != std::string::npos;
		const std::string cmd = changedResp ? args.substr(0, sp) : args;
		const std::string response = changedResp
			? args.substr(sp + 1) : "";
		if (!m_customCmds->editCom(cmd, response, cooldown)) {
			output += "invalid command: $" + cmd;
		} else if (!changedCd && !changedResp) {
			output += "command $" + cmd + " was unchanged.";
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
			output += "no response provided for command $"
				+ args + ".";
		} else {
			/* first word is command, rest is response */
			std::string cmd = args.substr(0, sp);
			std::string response = args.substr(sp + 1);
			if (!m_customCmds->addCom(cmd, response, cooldown))
				output += "invalid command name: $" + cmd;
			else
				output += "command $" + cmd
					+ " has been added with a " +
					std::to_string(cooldown) + "s cooldown.";
		}
	}
	return output;
}

std::string CommandHandler::delcomFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";
	if (!m_customCmds->isActive())
		return "Custom commands are currently disabled.";

	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() == 2)
		return "@" + c->nick + ", command $" + tokens[1]
			+ (m_customCmds->delCom(tokens[1])
			? " has been deleted." : " not found.");
	else
		return "Invalid syntax. Use \"$delcom COMMAND\".";
}

std::string CommandHandler::addrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::string output = "@" + c->nick + ", ";
	time_t cooldown = 300;
	int16_t opt;
	OptionParser op(c->fullCmd, "c:");
	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
		case 'c':
			try {
				cooldown = 60 * std::stoi(op.optarg());
			} catch (std::invalid_argument) {
				return output + "invalid number: " + op.optarg();
			}
			break;
		case '?':
			if (op.optopt() == 'c')
				return output + "no interval provided.";
			return output + "illegal option: " + (char)op.optopt();
		default:
			return "";
		}
	}

	if (cooldown % 300 != 0)
		return output + "cooldown must be a multiple of 5 minutes.";
	else if (cooldown > 3600)
		return output + "cooldown cannot be longer than an hour.";

	if (op.optind() >= c->fullCmd.length())
		output += "no message specified.";
	else if (!m_evtp->addMessage(c->fullCmd.substr(op.optind()), cooldown))
		output += "limit of 5 recurring messages reached.";
	else
		output += "recurring message \"" + c->fullCmd.substr(op.optind())
			+ "\" has been added at a "
			+ std::to_string(cooldown / 60) + " min interval.";
	return output;
}

std::string CommandHandler::delrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::string output = "@" + c->nick + ", ";
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);
	if (tokens.size() != 2)
		return output += "invalid syntax. Use \"$delrec ID\".";

	uint32_t id;
	try {
		id = std::stoi(tokens[1]);
	} catch (std::invalid_argument) {
		return output += "invalid number: " + tokens[1];
	}

	if (!m_evtp->delMessage(id))
		output += "invalid ID provided. Use \"$listrec\" to show "
			"all recurring message IDs.";
	else
		output += "recurring message " + std::to_string(id) + " deleted.";
	return output;
}

std::string CommandHandler::listrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";
	return m_evtp->messageList();
}

std::string CommandHandler::extractCMLData(const std::string &httpResp) const
{
	std::vector<std::string> elems;
	utils::split(httpResp, ',', elems);
	if (elems.size() == 4) {
		std::string ehp = elems[2];
		std::string::size_type dot;
		if ((dot = ehp.find(".")) != std::string::npos) {
			/* truncate to one decimal place */
			ehp = ehp.substr(0, dot + 2);
		}
		return "Name: " + elems[1] + ", Rank: " + elems[0] + ", EHP: "
			+ ehp + " (+" + elems[3] + " this week).";
	} else {
		return "Player either does not exist or has not been tracked on CML.";
	}
}

std::string CommandHandler::extractHSData(const std::string &httpResp, uint8_t skillID) const
{
	std::vector<std::string> skills;
	utils::split(httpResp, '\n', skills);
	
	std::vector<std::string> tokens;
	utils::split(skills[skillID], ',', tokens);

	return "Level: " + tokens[1] + ", Exp: "
		+ utils::formatInteger(tokens[2]) + ", Rank: "
		+ utils::formatInteger(tokens[0]) + ".";
}

std::string CommandHandler::extractGEData(const std::string &httpResp) const
{
	Json::Reader reader;
	Json::Value item;
	if (reader.parse(httpResp, item))
		return utils::formatInteger(item["overall"].asString());
	else
		return "An error occurred. Please try again.";
}
