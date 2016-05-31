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
#include "ExpressionParser.h"
#include "OptionParser.h"
#include "SkillMap.h"
#include "version.h"

#define DEFAULT 1
#define CUSTOM 2

CommandHandler::CommandHandler(const std::string &name,
		const std::string &channel, const std::string &token,
		Moderator *mod, URLParser *urlp, EventManager *evtp,
		Giveaway *givp)
	: m_name(name), m_channel(channel), m_token(token), m_modp(mod),
	m_parsep(urlp), m_customCmds(NULL), m_evtp(evtp), m_givp(givp),
	m_counting(false), m_gen(m_rd())
{
	/* initializing pointers to all default commands */
	m_defaultCmds["ehp"] = &CommandHandler::ehpFunc;
	m_defaultCmds["level"] = &CommandHandler::levelFunc;
	m_defaultCmds["lvl"] = &CommandHandler::levelFunc;
	m_defaultCmds["ge"] = &CommandHandler::geFunc;
	m_defaultCmds["calc"] = &CommandHandler::calcFunc;
	m_defaultCmds["cml"] = &CommandHandler::cmlFunc;
	m_defaultCmds["8ball"] = &CommandHandler::eightballFunc;
	m_defaultCmds["active"] = &CommandHandler::activeFunc;
	m_defaultCmds["uptime"] = &CommandHandler::uptimeFunc;
	m_defaultCmds["rsn"] = &CommandHandler::rsnFunc;
	m_defaultCmds["manual"] = &CommandHandler::manualFunc;
	m_defaultCmds["commands"] = &CommandHandler::manualFunc;
	m_defaultCmds["help"] = &CommandHandler::helpFunc;
	m_defaultCmds["about"] = &CommandHandler::aboutFunc;
	m_defaultCmds["submit"] = &CommandHandler::submitFunc;
	m_defaultCmds["duck"] = &CommandHandler::duckFunc;
	m_defaultCmds[m_wheel.cmd()] = &CommandHandler::wheelFunc;

	m_defaultCmds["sp"] = &CommandHandler::strawpollFunc;
	m_defaultCmds["strawpoll"] = &CommandHandler::strawpollFunc;
	m_defaultCmds["count"] = &CommandHandler::countFunc;
	m_defaultCmds["whitelist"] = &CommandHandler::whitelistFunc;
	m_defaultCmds["permit"] = &CommandHandler::permitFunc;
	m_defaultCmds["addcom"] = &CommandHandler::makecomFunc;
	m_defaultCmds["editcom"] = &CommandHandler::makecomFunc;
	m_defaultCmds["delcom"] = &CommandHandler::delcomFunc;
	m_defaultCmds["addrec"] = &CommandHandler::addrecFunc;
	m_defaultCmds["delrec"] = &CommandHandler::delrecFunc;
	m_defaultCmds["listrec"] = &CommandHandler::listrecFunc;
	m_defaultCmds["setrec"] = &CommandHandler::setrecFunc;
	m_defaultCmds["setgiv"] = &CommandHandler::setgivFunc;
	m_defaultCmds["status"] = &CommandHandler::statusFunc;

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

	populateHelp();
}

CommandHandler::~CommandHandler()
{
	delete m_customCmds;
}

std::string CommandHandler::processCommand(const std::string &nick,
	const std::string &fullCmd, bool privileges)
{
	std::string output = "";

	/* the command is the first part of the string up to the first space */
	std::string cmd = fullCmd.substr(0, fullCmd.find(' '));

	struct cmdinfo c;
	c.nick = nick;
	c.cmd = cmd;
	c.fullCmd = fullCmd;
	c.privileges = privileges;

	/* custom command */
	Json::Value *ccmd;

	switch (source(cmd)) {
	case DEFAULT:
		if (privileges || m_cooldowns.ready(cmd)) {
			output += (this->*m_defaultCmds[cmd])(&c);
			m_cooldowns.setUsed(cmd);
		} else {
			output += "/w " + nick + " command is on cooldown: "
				+ cmd;
		}
		break;
	case CUSTOM:
		ccmd = m_customCmds->getCom(cmd);
		if (privileges || m_cooldowns.ready((*ccmd)["cmd"].asString())) {
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

/* ehp: view players' ehp */
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
				output = c->cmd + ": illegal option -- ";
				output += (char)op.optopt();
				return output;
			default:
				return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (usenick)
			return c->cmd + ": must provide Twitch name for -n flag";
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
		return c->cmd + ": invalid syntax. Use \"$ehp [-n] [RSN]\"";
	std::replace(rsn.begin(), rsn.end(), '-', '_');

	cpr::Response resp = cpr::Get(cpr::Url("http://" + CML_HOST +
				CML_EHP_API + rsn), cpr::Header{{ "Connection", "close" }});
	if (resp.text == "-4")
		return c->cmd + ": could not reach CML API, try again";
	return "[EHP] " + extractCMLData(resp.text);
}

/* level: look up players' levels */
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
				output = c->cmd + ": illegal option -- ";
				output += (char)op.optopt();
				return output;
			default:
				return "";
		}
	}

	if (op.optind() == c->fullCmd.length())
		return c->cmd + ": invalid syntax. "
			"Use \"$level [-n] SKILL RSN\"";

	std::vector<std::string> argv;
	utils::split(c->fullCmd.substr(op.optind()), ' ', argv);

	if (argv.size() != 2)
		return c->cmd + ": invalid syntax. "
			"Use \"$level [-n] SKILL RSN\"";

	std::string skill = argv[0];
	std::string rsn, err;
	if ((rsn = getRSN(argv[1], c->nick, err, usenick)).empty())
		return err;
	std::replace(rsn.begin(), rsn.end(), '-', '_');

	if (skillMap.find(skill) == skillMap.end()
			&& skillNickMap.find(skill) == skillNickMap.end())
		return c->cmd + ": invalid skill name";

	uint8_t skillID = skillMap.find(skill) == skillMap.end()
		? skillNickMap.find(skill)->second
		: skillMap.find(skill)->second;

	cpr::Response resp = cpr::Get(cpr::Url("http://" + RS_HOST
				+ RS_HS_API + rsn),
			cpr::Header{{ "Connection", "close" }});
	if (resp.text.find("404 - Page not found") != std::string::npos)
		return c->cmd + ": player not found on hiscores";

	/* skill nickname is displayed to save space */
	std::string nick = getSkillNick(skillID);
	std::transform(nick.begin(), nick.end(), nick.begin(), toupper);

	return "[" + nick + "] Name: " + rsn + ", "
		+ extractHSData(resp.text, skillID);
}

/* ge: look up item prices */
std::string CommandHandler::geFunc(struct cmdinfo *c)
{
	if (!m_GEReader.active())
		return "";
	if (c->fullCmd.length() < 4)
		return c->cmd + ": no item name provided";

	std::string itemName = c->fullCmd.substr(3);
	std::replace(itemName.begin(), itemName.end(), '_', ' ');

	Json::Value item = m_GEReader.getItem(itemName);
	if (item.empty())
		return c->cmd + ": item not found: " + itemName;

	cpr::Response resp = cpr::Get(cpr::Url("http://" + EXCHANGE_HOST
		+ EXCHANGE_API + item["id"].asString()),
		cpr::Header{{ "Connection", "close" }});
	return "[GE] " + item["name"].asString() + ": "
		+ extractGEData(resp.text) + " gp";
}

/* calc: perform basic calculations */
std::string CommandHandler::calcFunc(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6)
		return c->cmd + ": invalid mathematical expression";

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
		return c->cmd + ": division by 0";

	return "[CALC] " + result.str();
}

/* cml: interact with crystalmathlabs trackers */
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
			output = c->cmd + ": illegal option -- ";
			output += (char)op.optopt();
			return output;
		default:
			return "";
		}
	}

	if (op.optind() == c->fullCmd.length()) {
		if (update)
			return c->cmd + ": must provide RSN for -u flag";
		if (usenick)
			return c->cmd + ": must provide Twitch name for -n flag";
		else
			return "[CML] http://" + CML_HOST;
	}

	std::string rsn, err;
	if ((rsn = getRSN(c->fullCmd.substr(op.optind()),
			c->nick, err, usenick)).empty())
		return err;
	if (rsn.find(' ') != std::string::npos)
		return output += "invalid syntax. Use \"$cml [-nu] [RSN]\"";
	
	if (update) {
		cpr::Response resp = cpr::Get(
				cpr::Url("http://" + CML_HOST + CML_UPDATE_API + rsn),
				cpr::Header{{ "Connection", "close" }});
		uint8_t i = std::stoi(resp.text);
		switch (i) {
		case 1:
			output += rsn + "'s CML was updated";
			break;
		case 2:
			output += rsn + " could not be found on the hiscores";
			break;
		case 3:
			output += rsn + " has had a negative XP gain";
			break;
		case 4:
			output += "unknown error, try again";
			break;
		case 5:
			output += rsn + " has been updated within the last 30s";
			break;
		case 6:
			output += rsn + " is an invalid RSN";
			break;
		default:
			output += "could not reach CML API, try again";
			break;
		}
		return output;
	} else {
		return "[CML] http://" + CML_HOST + CML_USER + rsn;
	}
}

/* wheel: select items from various categories */
std::string CommandHandler::wheelFunc(struct cmdinfo *c)
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

/* 8ball: respond to questions */
std::string CommandHandler::eightballFunc(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6
			|| c->fullCmd[c->fullCmd.length() - 1] != '?')
		return "[8 BALL] Ask me a question.";
	std::uniform_int_distribution<> dis(0, m_eightballResponses.size());
	return "[8 BALL] @" + c->nick + ", "
		+ m_eightballResponses[dis(m_gen)] + ".";
}

/* strawpoll: create polls */
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
			output = c->cmd + ": illegal option -- "; 
			output += (char)op.optopt();
			return output;
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
			output += "Poll created : " + m_activePoll;
		}
	} else {
		return c->cmd + ": error connecting to server";
	}

	return output;
}

/* active: view current poll */
std::string CommandHandler::activeFunc(struct cmdinfo *c)
{
	return "[ACTIVE] @" + c->nick + ", " + (m_activePoll.empty()
		? "no poll has been created." : m_activePoll);
}

/* commands: view default bot commands */
std::string CommandHandler::manualFunc(struct cmdinfo *c)
{
	return "[MANUAL] @" + c->nick + ", " + BOT_WEBSITE
		+ "/manual/index.html";
}

/* help: view command reference manuals */
std::string CommandHandler::helpFunc(struct cmdinfo *c)
{
	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() != 2)
		return c->cmd + ": invalid syntax. Use \"$help CMD\"";

	std::string path = BOT_WEBSITE + "/manual/";
	if (m_help.find(argv[1]) != m_help.end())
		return "[HELP] " + path + m_help[argv[1]] + ".html";

	if (m_defaultCmds.find(argv[1]) != m_defaultCmds.end())
		return "[HELP] " + path + argv[1] + ".html";

	Json::Value *ccmd;
	if (!(ccmd = m_customCmds->getCom(argv[1]))->empty())
		return "[HELP] " + argv[1] + " is a custom command";

	return "[HELP] Not a bot command: " + argv[1];
}

/* about: print bot information */
std::string CommandHandler::aboutFunc(struct cmdinfo *c)
{
	return "[ABOUT] @" + c->nick + ", " + m_name + " is running "
		+ BOT_NAME + " v" + BOT_VERSION + ". Find out more at "
		+ BOT_WEBSITE;
}

/* count: manage message counts */
std::string CommandHandler::countFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() != 2 || !(argv[1] == "start" || argv[1] == "stop"
		|| argv[1] == "display"))
		return c->cmd + ": invalid syntax. "
			"Use \"$count start|stop|display\"";

	if (argv[1] == "start") {
		/* begin a new count */
		if (m_counting)
			return "A count is already running. "
				"End it before starting a new one.";
		m_usersCounted.clear();
		m_messageCounts.clear();
		m_counting = true;
		return "Message counting has begun. Prepend your message with "
			"a + to have it counted.";
	} else if (argv[1] == "stop") {
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
				[=](mcount &a, mcount &b) {
					return a.second > b.second;
				});
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

/* uptime: check how long channel has been live */
std::string CommandHandler::uptimeFunc(struct cmdinfo *c)
{
	/* don't believe me just watch */
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

/* rsn: view and manage stored rsns */
std::string CommandHandler::rsnFunc(struct cmdinfo *c)
{
	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() < 2 || (argv[1] != "set" && argv[1] != "check"
				&& argv[1] != "del" && argv[1] != "change")
			|| (argv[1] == "set" && argv.size() != 3)
			|| (argv[1] == "check" && argv.size() > 3)
			|| (argv[1] == "del" && argv.size() != 2)
			|| (argv[1] == "change" && argv.size() != 3))
		return "Invalid syntax. Use \"$rsn set RSN\", \"$rsn del\", "
			"\"$rsn change RSN\" or \"$rsn check [NICK]\"";

	std::string err, rsn;
	if (argv.size() > 2) {
		rsn = argv[2];
		std::transform(rsn.begin(), rsn.end(), rsn.begin(), tolower);
	}
	if (argv[1] == "set") {
		if (!m_rsns.add(c->nick, rsn, err))
			return "@" + c->nick + ", " + err;
		else
			return "RSN " + rsn + " has been set for "
				+ c->nick + ".";
	} else if (argv[1] == "del") {
		if (!m_rsns.del(c->nick))
			return "@" + c->nick + ", you do not have a RSN set.";
		else
			return "RSN for " + c->nick + " has been deleted.";
	} else if (argv[1] == "change") {
		if (!m_rsns.edit(c->nick, rsn, err))
			return "@" + c->nick + ", " + err;
		else
			return "RSN for " + c->nick + " changed to "
				+ rsn + ".";
	} else {
		/* check own nick or the one that was given */
		std::string crsn, nick;
		nick = argv.size() == 2 ? c->nick : argv[2];
		if ((crsn = m_rsns.getRSN(nick)).empty())
			return "No RSN set for " + nick + ".";
		else
			return "RSN \"" + crsn + "\" is currently set for "
				+ nick + ".";
	}
}

/* submit: submit a message to the streamer */
std::string CommandHandler::submitFunc(struct cmdinfo *c)
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

/* duck: search duckduckgo with a query string */
std::string CommandHandler::duckFunc(struct cmdinfo *c)
{
	if (c->fullCmd.length() < 6)
		return c->cmd + ": must provide search term";

	std::string search = c->fullCmd.substr(5);
	return "https://duckduckgo.com/?q=" + tw::pencode(search);
}

/* whitelist: exempt websites from moderation */
std::string CommandHandler::whitelistFunc(struct cmdinfo *c)
{
	if (!c->privileges)
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

	return "@" + c->nick + ", invalid URL: " + argv[1];
}

/* permit: grant user permission to post a url */
std::string CommandHandler::permitFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() != 2)
		return c->cmd + ": invalid syntax. Use \"$permit USER\"";

	m_modp->permit(argv[1]);
	return argv[1] + " has been granted permission to post a link.";
}

/* makecom: create or modify a custom command */
std::string CommandHandler::makecomFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	if (!m_customCmds->isActive())
		return "Custom commands are currently disabled.";

	bool editing = c->cmd == "editcom";
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
					return c->cmd + ": cooldown cannot be "
						"negative";
			} catch (std::invalid_argument) {
				output = c->cmd  + ": invalid number -- ";
				return output + op.optarg();
			}
			break;
		case '?':
			/* invalid option */
			if (op.optopt() == 'c') {
				output = c->cmd + ": -c option given without "
					"cooldown";
				return output;
			} else {
				output = c->cmd + ": illegal option -- ";
				output += (char)op.optopt();
				return output;
			}
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
		const std::string response = changedResp
			? args.substr(sp + 1) : "";
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

/* delcom: delete a custom command */
std::string CommandHandler::delcomFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";
	if (!m_customCmds->isActive())
		return "Custom commands are currently disabled.";

	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);

	if (argv.size() == 2)
		return "@" + c->nick + ", command $" + argv[1]
			+ (m_customCmds->delCom(argv[1])
			? " has been deleted." : " not found.");
	else
		return c->cmd + ": invalid syntax. Use \"$delcom COMMAND\"";
}

/* addrec: add a recurring message */
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
				return c->cmd + ": invalid number -- "
					+ op.optarg();
			}
			break;
		case '?':
			if (op.optopt() == 'c')
				return c->cmd + "-c flag given without interval";
			output = c->cmd + ": illegal option -- ";
			output += (char)op.optopt();
			return output;
		default:
			return "";
		}
	}

	if (cooldown % 300 != 0)
		return c->cmd + ": interval must be a multiple of 5 minutes";
	else if (cooldown > 3600)
		return c->cmd + ": interval cannot be longer than an hour";

	if (op.optind() >= c->fullCmd.length())
		return c->cmd + ": no message specified";
	else if (!m_evtp->addMessage(c->fullCmd.substr(op.optind()), cooldown))
		return c->cmd + ": limit of 5 recurring messages reached";
	else
		output += "recurring message \""
			+ c->fullCmd.substr(op.optind())
			+ "\" has been added at a "
			+ std::to_string(cooldown / 60) + " min interval.";
	return output;
}

/* delrec: delete a recurring message */
std::string CommandHandler::delrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";

	std::string output = "@" + c->nick + ", ";
	std::vector<std::string> argv;
	utils::split(c->fullCmd, ' ', argv);
	if (argv.size() != 2)
		return c->cmd + ": invalid syntax. Use \"$delrec ID\"";

	uint32_t id;
	try {
		id = std::stoi(argv[1]);
	} catch (std::invalid_argument) {
		return c->cmd + ": invalid number -- " + argv[1];
	}

	if (!m_evtp->delMessage(id))
		return c->cmd + "invalid ID provided. Use \"$listrec\" to show "
			"all recurring message IDs.";
	else
		output += "recurring message " + std::to_string(id)
			+ " deleted.";
	return output;
}

/* listrec: show all recurring messages */
std::string CommandHandler::listrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";
	return m_evtp->messageList();
}

/* setrec: enable and disable recurring messages */
std::string CommandHandler::setrecFunc(struct cmdinfo *c)
{
	if (!c->privileges)
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

/* setgiv: change giveaway settings */
std::string CommandHandler::setgivFunc(struct cmdinfo *c)
{
	std::string output = "@" + c->nick + ", ";
	OptionParser op(c->fullCmd, "fn:t");
	int16_t opt;
	int32_t amt = 0;
	bool setfollowers = false, settimer = false;
	
	while ((opt = op.getopt()) != EOF) {
		switch (opt) {
		case 'f':
			setfollowers = true;
			break;
		case 'n':
			try {
				if ((amt = std::stoi(op.optarg())) <= 0)
					return c->cmd + ": amount must be a "
						"positive integer";
			} catch (std::invalid_argument) {
				return c->cmd + ": invalid number -- "
					+ op.optarg();
			}
			break;
		case 't':
			settimer = true;
			break;
		case '?':
			if (op.optopt() == 'n')
				return c->cmd + ": -n option given "
					"without amount";
			output = c->cmd + ": illegal option -- ";
			output += (char)op.optopt();
			return output;
		default:
			return "";
		}
	}

	if (setfollowers && settimer)
		return c->cmd + ": cannot combine -f and -t flags";

	if (op.optind() == c->fullCmd.length())
		return c->cmd + ": invalid syntax. Use \"$setgiv [-ft] "
			"[-n AMOUNT] on|off|check\"";

	std::string setting = c->fullCmd.substr(op.optind());
	std::string err, res;

	/* allow all users to check but only moderators to set */
	if (setting == "check") {
		int8_t type = -1;
		if (setfollowers)
			type = 1;
		if (settimer)
			type = 2;
		return output + m_givp->currentSettings(type);
	}
	if (!c->privileges)
		return output + "you do not have permission to perform "
			"this action.";

	if (setting == "on") {
		if (setfollowers) {
			m_givp->setFollowers(true, amt);
			res = "giveaways set to occur every ";
			res += std::to_string(m_givp->followers());
			res += " followers.";
		} else if (settimer) {
			m_givp->setTimer(true, (time_t)amt * 60);
			res = "giveaways set to occur every ";
			res += std::to_string(m_givp->interval() / 60);
			res += " minutes.";
		} else {
			m_givp->activate(time(nullptr), err);
			res = "giveaways have been activated.";
		}
	} else if (setting == "off") {
		if (setfollowers) {
			m_givp->setFollowers(false);
			res = "follower giveaways have been disabled.";
		} else if (settimer) {
			m_givp->setTimer(false);
			res = "timed giveaways have been disabled.";
		} else {
			m_givp->deactivate();
			res = "giveaways have been deactivated.";
		}
	} else {
		return c->cmd + ": invalid syntax. Use \"$setgiv [-ft] "
			"[-n AMOUNT] on|off|check\"";
	}
	if (!err.empty())
		return output + err;

	return output + res;
}

/* status: set channel status */
std::string CommandHandler::statusFunc(struct cmdinfo *c)
{
	if (!c->privileges)
		return "";
	if (c->fullCmd.length() < 8)
		return c->cmd + ": no status given";

	std::string status = c->fullCmd.substr(7);
	Json::Value response;
	std::replace(status.begin(), status.end(), ' ', '+');
	std::string content = "channel[status]=" + status;

	cpr::Response resp = cpr::Put(cpr::Url(TWITCH_API + "/channels/"
		+ m_channel), cpr::Body(content), cpr::Header{
		{ "Accept", "application/vnd.twitchtv.v3+json" },
		{ "Authorization", "OAuth " + m_token }});

	Json::Reader reader;
	if (reader.parse(resp.text, response)) {
		if (response.isMember("error"))
			return c->cmd + ": " + response["error"].asString();
		return "[STATUS] Channel status changed to \""
			+ response["status"].asString() + "\".";
	}
	return c->cmd + ": something went wrong, try again";
}

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
		return "Player either does not exist or has not "
			"been tracked on CML.";
	}
}

std::string CommandHandler::extractHSData(const std::string &httpResp,
	uint8_t skillID) const
{
	std::vector<std::string> skills;
	utils::split(httpResp, '\n', skills);
	
	std::vector<std::string> argv;
	utils::split(skills[skillID], ',', argv);

	return "Level: " + argv[1] + ", Exp: "
		+ utils::formatInteger(argv[2]) + ", Rank: "
		+ utils::formatInteger(argv[0]) + ".";
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

void CommandHandler::populateHelp()
{
	m_help[m_wheel.cmd()] = "selection-wheel";
	m_help["wheel"] = "selection-wheel";
	m_help["lvl"] = "level";
	m_help["sp"] = "strawpoll";
}
