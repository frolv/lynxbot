#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <json/json.h>
#include <cpr/cpr.h>
#include "CommandHandler.h"
#include "ExpressionParser.h"
#include "OptionParser.h"
#include "SkillMap.h"
#include "utils.h"
#include "version.h"

CommandHandler::CommandHandler(const std::string &name,
		const std::string &channel, Moderator *mod,
		URLParser *urlp, EventManager *evtp, Giveaway *givp)
	: m_name(name), m_channel(channel), m_modp(mod), m_parsep(urlp),
	m_customCmds(NULL), m_evtp(evtp), m_givp(givp), m_counting(false),
	m_gen(m_rd())
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
	m_defaultCmds["strawpoll"] = &CommandHandler::strawpollFunc;
	m_defaultCmds["active"] = &CommandHandler::activeFunc;
	m_defaultCmds["count"] = &CommandHandler::countFunc;
	m_defaultCmds["uptime"] = &CommandHandler::uptimeFunc;
	m_defaultCmds["rsn"] = &CommandHandler::rsnFunc;
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
	m_defaultCmds["setrec"] = &CommandHandler::setrecFunc;
	m_defaultCmds["setgiv"] = &CommandHandler::setgivFunc;

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
	std::string output = "@" + c->nick + ", ";
	OptionParser op(c->fullCmd, "n");
	int16_t opt;
	bool usenick = false;

	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
			case 'n':
				usenick = true;
				break;
			case '?':
				output = "ehp: illegal option -- ";
				output += (char)op.optopt();
				return output;
			default:
				return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (usenick)
			return output += "must provide Twitch name for -n flag.";
		else
			return "[EHP] EHP stands for efficient hours played. "
				"You earn 1 EHP whenever you gain a certain "
				"amount of experience in a skill, depending "
				"on your level. You can find XP rates here: "
				"http://crystalmathlabs.com/tracker/suppliescalc.php . "
				"Watch a video explaining EHP: "
				"https://www.youtube.com/watch?v=rhxHlO8mvpc";
	}

	std::string rsn, err;
	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return err;
	if (rsn.find(' ') != std::string::npos)
		return output += "invalid syntax. Use \"$ehp [-n] [RSN]\".";
	std::replace(rsn.begin(), rsn.end(), '-', '_');

	cpr::Response resp = cpr::Get(cpr::Url("http://" + CML_HOST +
				CML_EHP_API + rsn), cpr::Header{{ "Connection", "close" }});
	if (resp.text == "-4")
		return "@" + c->nick + ", could not reach CML API, try again.";
	return "[EHP] " + extractCMLData(resp.text);
}

std::string CommandHandler::levelFunc(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	OptionParser op(c->fullCmd, "n");
	int16_t opt;
	bool usenick = false;

	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
			case 'n':
				usenick = true;
				break;
			case '?':
				output = "ehp: illegal option -- ";
				output += (char)op.optopt();
				return output;
			default:
				return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return output + "invalid syntax. Use \"$level [-n] SKILL RSN\".";

	std::vector<std::string> tokens;
	utils::split(c->fullCmd.substr(op.optind()), ' ', tokens);

	if (tokens.size() != 2)
		return output + "invalid syntax. Use \"$level [-n] SKILL RSN\".";

	std::string skill = tokens[0];
	std::string rsn, err;
	if ((rsn = getRSN(tokens[1], c->nick, err, usenick)).empty())
		return err;
	std::replace(rsn.begin(), rsn.end(), '-', '_');

	if (skillMap.find(skill) == skillMap.end()
			&& skillNickMap.find(skill) == skillNickMap.end())
		return "[LVL] Invalid skill name.";

	uint8_t skillID = skillMap.find(skill) == skillMap.end()
		? skillNickMap.find(skill)->second
		: skillMap.find(skill)->second;

	cpr::Response resp = cpr::Get(cpr::Url("http://" + RS_HOST
				+ RS_HS_API + rsn), cpr::Header{{ "Connection", "close" }});
	if (resp.text.find("404 - Page not found") != std::string::npos)
		return "[LVL] Player not found on hiscores.";

	/* skill nickname is displayed to save space */
	std::string nick = getSkillNick(skillID);
	std::transform(nick.begin(), nick.end(), nick.begin(), toupper);

	return "[" + nick + "] Name: " + rsn + ", "
		+ extractHSData(resp.text, skillID);
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
	
	std::ostringstream result;
	try {
		ExpressionParser exprP(expr);
		exprP.tokenizeExpr();
		double res = exprP.eval();
		result << res;
	} catch (std::runtime_error &e) {
		return e.what();
	}
	if (result.str() == "inf" || result.str() == "-nan(ind)")
		return "Error: division by 0.";

	return "[CALC] " + result.str();
}

std::string CommandHandler::cmlFunc(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	OptionParser op(c->fullCmd, "nu");
	int16_t opt;
	bool usenick = false, update = false;

	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
		case 'n':
			usenick = true;
			break;
		case 'u':
			update = true;
			break;
		case '?':
			output = "cml: illegal option -- ";
			output += (char)op.optopt();
			return output;
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (update)
			return output + "must provide RSN for -u flag.";
		else
			return "[CML] http://" + CML_HOST;
	}

	std::string rsn, err;
	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return err;
	if (rsn.find(' ') != std::string::npos)
		return output += "invalid syntax. Use \"$cml [-nu] [RSN]\".";
	
	if (update) {
		cpr::Response resp = cpr::Get(
				cpr::Url("http://" + CML_HOST + CML_UPDATE_API + rsn),
				cpr::Header{{ "Connection", "close" }});
		uint8_t i = std::stoi(resp.text);
		switch (i) {
		case 1:
			output += rsn + "'s CML was updated.";
			break;
		case 2:
			output += rsn + " could not be found on the hiscores.";
			break;
		case 3:
			output += rsn + " has had a negative XP gain.";
			break;
		case 4:
			output += "unknown error, try again.";
			break;
		case 5:
			output += rsn + " has been updated within the last 30s.";
			break;
		case 6:
			output += rsn + " is an invalid RSN.";
			break;
		default:
			output += "could not reach CML API, try again.";
			break;
		}
		return output;
	} else {
		return "[CML] http://" + CML_HOST + CML_USER + rsn;
	}
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
			output = "strawpoll: illegal option -- "; 
			output += (char)op.optopt();
			return output;
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
	if (resp.text.substr(0, 7) == "Channel")
		return out + channel + " is not currently live.";
	else
		return out + channel + " has been live for " + resp.text + ".";
}

std::string CommandHandler::rsnFunc(struct cmdinfo *c)
{
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() < 2 || (tokens[1] != "set" && tokens[1] != "check"
				&& tokens[1] != "del" && tokens[1] != "change")
			|| (tokens[1] == "set" && tokens.size() != 3)
			|| (tokens[1] == "check" && tokens.size() > 3)
			|| (tokens[1] == "del" && tokens.size() != 2)
			|| (tokens[1] == "change" && tokens.size() != 3))
		return "Invalid syntax. Use \"$rsn set RSN\", \"$rsn del\", "
			"\"$rsn change RSN\" or \"$rsn check [NICK]\".";

	std::string err, rsn;
	if (tokens.size() > 2) {
		rsn = tokens[2];
		std::transform(rsn.begin(), rsn.end(), rsn.begin(), tolower);
	}
	if (tokens[1] == "set") {
		if (!m_rsns.add(c->nick, rsn, err))
			return "@" + c->nick + ", " + err;
		else
			return "RSN " + rsn + " has been set for "
				+ c->nick + ".";
	} else if (tokens[1] == "del") {
		if (!m_rsns.del(c->nick))
			return "@" + c->nick + ", you do not have a RSN set.";
		else
			return "RSN for " + c->nick + " has been deleted.";
	} else if (tokens[1] == "change") {
		if (!m_rsns.edit(c->nick, rsn, err))
			return "@" + c->nick + ", " + err;
		else
			return "RSN for " + c->nick + " changed to "
				+ rsn + ".";
	} else {
		/* check own nick or the one that was given */
		std::string crsn, nick;
		nick = tokens.size() == 2 ? c->nick : tokens[2];
		if ((crsn = m_rsns.getRSN(nick)).empty())
			return "No RSN set for " + nick + ".";
		else
			return "RSN \"" + crsn + "\" is currently set for "
				+ nick + ".";
	}
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
			if (op.optopt() == 'c') {
				output += "no cooldown provided.";
			} else {
				output = editing ? "edit" : "add" ;
				return output + "com: illegal option -- "
					+ (char)op.optopt();
			}
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
			output = "addrec: illegal option -- ";
			output += (char)op.optopt();
			return output;
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

std::string CommandHandler::setrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::string output = "@" + c->nick + ", ";
	std::vector<std::string> tokens;
	utils::split(c->fullCmd, ' ', tokens);

	if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off"))
		return output + "invalid syntax. Use \"setrec on|off\".";

	if (tokens[1] == "on") {
		m_evtp->activateMessages();
		output += "recurring messages enabled.";
	} else {
		m_evtp->deactivateMessages();
		output += "recurring messages disabled.";
	}
	return output;
}

/* change giveaway settings while the bot is active */
std::string CommandHandler::setgivFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::string output = "@" + c->nick + ", ";
	OptionParser op(c->fullCmd, "fn:t");
	int16_t opt;
	int32_t amt;
	bool setfollowers = false, settimer = false;
	
	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
		case 'f':
			setfollowers = true;
			break;
		case 'n':
			try {
				amt = std::stoi(op.optarg());
			} catch (std::invalid_argument) {
				return "setgiv: invalid number -- "
					+ op.optarg();
			}
			break;
		case 't':
			settimer = true;
			break;
		case '?':
			if (op.optopt() == 'n')
				return "setgiv: -n option given without amount";
			output = "setgiv: illegal option -- ";
			output += (char)op.optopt();
			return output;
		default:
			return "";
		}
	}

	if (setfollowers && settimer)
		return "setgiv: cannot combine -f and -t flags";

	if (op.optind() == c->fullCmd.length())
		return output + "invalid syntax. Use \"$setgiv [-ft] "
			"[-n AMOUNT] on|off\".";

	std::string setting = c->fullCmd.substr(op.optind());
	std::string err, res;
	if (setting == "on") {
		if (setfollowers) {
		} else if (settimer) {
		} else {
			m_givp->activate(time(nullptr), err);
			res = "giveaways have been activated.";
		}
	} else if (setting == "off") {
		if (setfollowers) {
		} else if (settimer) {
		} else {
			m_givp->deactivate();
			res = "giveaways have been deactivated.";
		}
	} else {
		return output + "invalid syntax. Use \"$setgiv [-ft] "
			"[-n AMOUNT] on|off\".";
	}
	if (!err.empty())
		return output + err;

	return output + res;
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

std::string CommandHandler::extractHSData(const std::string &httpResp,
	uint8_t skillID) const
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
