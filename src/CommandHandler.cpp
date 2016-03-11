#include <json/json.h>
#include <cpr/cpr.h>
#include <ExpressionParser.h>
#include "stdafx.h"
#include "OptionParser.h"
#include "cmdmodules/SkillMap.h"

CommandHandler::CommandHandler(const std::string &name, Moderator *mod, URLParser *urlp, EventManager *evtp)
	:m_name(name), m_modp(mod), m_parsep(urlp), m_customCmds(&m_defaultCmds, &m_cooldowns, m_wheel.cmd()),
	m_counting(false), m_evtp(evtp), m_gen(m_rd())
{
	// initializing pointers to all default commands
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

	if (!m_customCmds.isActive()) {
		std::cerr << " Custom commands will be disabled for this session." << std::endl;
		std::cin.get();
	}

	// read all responses from file
	m_responding = utils::readJSON("responses.json", m_responses);
	if (m_responding) {
		// add response cooldowns to TimerManager
		for (auto &val : m_responses["responses"]) {
			m_cooldowns.add(val["name"].asString(), val["cooldown"].asInt());
		}
	}
	else {
		std::cerr << "Failed to read responses.json. Responses disabled for this session.";
		std::cin.get();
	}

	// set all command cooldowns
	for (auto &p : m_defaultCmds) {
		m_cooldowns.add(p.first);
	}
	m_cooldowns.add(m_wheel.name(), 10);

	// read extra 8ball responses
	std::ifstream reader(utils::getApplicationDirectory() + "/extra8ballresponses.txt");
	if (!reader.is_open()) {
		std::cerr << "Could not read extra8ballresponses.txt" << std::endl;
	}
	std::string line;
	while (std::getline(reader, line)) {
		m_eightballResponses.push_back(line);
	}
	reader.close();
}

CommandHandler::~CommandHandler() {}

std::string CommandHandler::processCommand(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	std::string output = "";

	// the command is the first part of the string up to the first space
	std::string cmd = fullCmd.substr(0, fullCmd.find(' '));

	if (m_defaultCmds.find(cmd) != m_defaultCmds.end() && (privileges || m_cooldowns.ready(cmd))) {
		output += (this->*m_defaultCmds[cmd])(nick, fullCmd, privileges);
		m_cooldowns.setUsed(cmd);
	}
	else if (m_wheel.isActive() && cmd == m_wheel.cmd() && (privileges || m_cooldowns.ready(m_wheel.name()))) {
		output += wheelFunc(nick, fullCmd, privileges);
		m_cooldowns.setUsed(m_wheel.name());
	}
	else if (m_customCmds.isActive()) {
		/* check for custom command */
		Json::Value *customCmd = m_customCmds.getCom(cmd);
		if (!customCmd->empty() && (privileges || m_cooldowns.ready((*customCmd)["cmd"].asString()))) {
			output += (*customCmd)["response"].asString();
			m_cooldowns.setUsed((*customCmd)["cmd"].asString());
		}
		else {
			std::cerr << "Invalid command or is on cooldown: " << cmd << std::endl << std::endl;
		}
	}

	return output;
}

std::string CommandHandler::processResponse(const std::string &message)
{
	if (!m_responding) {
		return "";
	}

	/* test the message against all response regexes */
	for (auto &val : m_responses["responses"]) {
		std::string name = val["name"].asString();
		std::string regex = val["regex"].asString();

		std::regex responseRegex(regex, std::regex_constants::ECMAScript | std::regex_constants::icase);
		std::smatch match;
		if (std::regex_search(message.begin(), message.end(), match, responseRegex) && m_cooldowns.ready(name)) {
			m_cooldowns.setUsed(name);
			return val["response"].asString();
		}

	}
	// no match
	return "";
}

bool CommandHandler::isCounting() const
{
	return m_counting;
}

void CommandHandler::count(const std::string &nick, std::string &message)
{
	std::transform(message.begin(), message.end(), message.begin(), tolower);

	// if nick is not found
	if (std::find(m_usersCounted.begin(), m_usersCounted.end(), nick) == m_usersCounted.end()) {
		m_usersCounted.push_back(nick);
		if (m_messageCounts.find(message) == m_messageCounts.end()) {
			m_messageCounts.insert({ message, 1 });
		}
		else {
			m_messageCounts.find(message)->second++;
		}
	}
}

std::string CommandHandler::ehpFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() == 2) {
		// a username was provided
		std::string rsn = tokens[1];
		std::replace(rsn.begin(), rsn.end(), '-', '_');
		cpr::Response resp = cpr::Get(cpr::Url("http://" + CML_HOST + CML_EHP_API + rsn), cpr::Header{{ "Connection", "close" }});
		return "[EHP] " + extractCMLData(resp.text, rsn);

	}
	else if (tokens.size() == 1) {
		return "[EHP] EHP stands for efficient hours played. You earn 1 EHP whenever you gain a certain amount of experience in a skill, \
			depending on your level. You can find XP rates here: http://crystalmathlabs.com/tracker/suppliescalc.php";
	}
	else {
		return "[EHP] Invalid syntax. Use \"$ehp [RSN]\".";
	}
}

std::string CommandHandler::levelFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() == 3) {

		if (skillMap.find(tokens[1]) == skillMap.end() && skillNickMap.find(tokens[1]) == skillNickMap.end()) {
			return "[LVL] Invalid skill name.";
		}
		uint8_t skillID = skillMap.find(tokens[1]) == skillMap.end() ? skillNickMap.find(tokens[1])->second : skillMap.find(tokens[1])->second;

		std::string rsn = tokens[2];
		std::replace(rsn.begin(), rsn.end(), '-', '_');

		cpr::Response resp = cpr::Get(cpr::Url("http://" + RS_HOST + RS_HS_API + rsn), cpr::Header{{ "Connection", "close" }});
		if (resp.text.find("404 - Page not found") != std::string::npos) {
			return "[LVL] Player not found on hiscores.";
		}

		// skill nickname is displayed to save space
		std::string nick = getSkillNick(skillID);
		std::transform(nick.begin(), nick.end(), nick.begin(), ::toupper);

		return "[" + nick + "] Name: " + rsn + ", " + extractHSData(resp.text, skillID);
	}
	else {
		return "[LVL] Invalid syntax. Use \"$lvl SKILL RSN\".";
	}
}

std::string CommandHandler::geFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!m_GEReader.active()) {
		return "";
	}
	if (fullCmd.length() < 4) {
		return "[GE] No item name provided.";
	}

	std::string itemName = fullCmd.substr(3);
	std::replace(itemName.begin(), itemName.end(), '_', ' ');

	Json::Value item = m_GEReader.getItem(itemName);
	if (item.empty()) {
		return "[GE] Item not found: " + itemName + ".";
	}

	cpr::Response resp = cpr::Get(cpr::Url("http://" + EXCHANGE_HOST + EXCHANGE_API + item["id"].asString()), cpr::Header{{ "Connection", "close" }});
	return "[GE] " + item["name"].asString() + ": " + extractGEData(resp.text) + " gp.";
}

std::string CommandHandler::calcFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (fullCmd.length() < 6) {
		return "Invalid mathematical expression.";
	}

	std::string expr = fullCmd.substr(5);
	// remove all whitespace
	expr.erase(std::remove_if(expr.begin(), expr.end(), isspace), expr.end());
	
	std::string result;
	try {
		ExpressionParser exprP(expr);
		exprP.tokenizeExpr();
		double res = exprP.eval();
		result += std::to_string(res);
	}
	catch (std::runtime_error &e) {
		result = e.what();
	}
	if (result == "inf" || result == "-nan(ind)") {
		result = "Error: division by 0.";
	}

	return "[CALC] " + result;
}

std::string CommandHandler::cmlFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	return "[CML] http://" + CML_HOST;
}

std::string CommandHandler::wheelFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() == 1) {
		return m_wheel.name() + ": " + m_wheel.desc() + " " + m_wheel.usage();
	}
	if (tokens.size() > 2 || (!m_wheel.valid(tokens[1]) && tokens[1] != "check")) {
		return "Invalid syntax. " + m_wheel.usage();
	}
	std::string output = "@" + nick + ", ";

	if (tokens[1] == "check") {
		// return the current selection
		bool ready = m_wheel.ready(nick);
		output += ready ? "you are not currently assigned anything." : "you are currently assigned " + m_wheel.selection(nick) + ".";
	}
	else if (!m_wheel.ready(nick)) {
		output += "you have already been assigned something!";
	}
	else {
		// make a new selection
		output += "your entertainment for tonight is " + m_wheel.choose(nick, tokens[1]) + ".";
	}

	return output;
}

std::string CommandHandler::eightballFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (fullCmd.length() < 6 || fullCmd[fullCmd.length() - 1] != '?') {
		return "[8 BALL] Ask me a question.";
	}
	std::uniform_int_distribution<> dis(0, m_eightballResponses.size());
	return "[8 BALL] @" + nick + ", " + m_eightballResponses[dis(m_gen)] + ".";
}

std::string CommandHandler::strawpollFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	// json values to hold created poll, poll options and http response
	Json::Value poll, options(Json::arrayValue), response;
	std::string output = "[SPOLL] ";
	bool binary = false, multi = false, captcha = false;

	int16_t c;
	OptionParser op(fullCmd, "bcm");
	while ((c = op.getopt()) != EOF) {
		switch (c) {
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

	if (op.optind() >= fullCmd.length()) {
		return output + "Not enough arguments given.";
	}
	const std::string req = fullCmd.substr(op.optind());
	std::vector<std::string> tokens;
	utils::split(req, '|', tokens);

	if (binary && tokens.size() != 1) {
		return output + "Cannot provide options for binary poll.";
	}
	if (!binary && tokens.size() < 3) {
		return output + "Poll must have a question and at least two answers.";
	}

	if (binary) {
		options.append("yes");
		options.append("no");
	}
	else {
		for (size_t i = 1; i < tokens.size(); ++i) {
			options.append(tokens[i]);
		}
	}

	// populate the poll json
	poll["title"] = tokens[0];
	poll["options"] = options;
	poll["captcha"] = captcha;
	poll["multi"] = multi;

	// format and post the poll
	Json::FastWriter fw;
	const std::string content = fw.write(poll);
	cpr::Response resp = cpr::Post(cpr::Url("http://" + STRAWPOLL_HOST + STRAWPOLL_API), cpr::Body(content),
		cpr::Header{ { "Connection", "close" }, { "Content-Type", "application/json" }, { "Content-Length", std::to_string(content.length()) } });

	Json::Reader reader;
	if (reader.parse(resp.text, response)) {
		if (!response.isMember("id")) {
			output += "Poll could not be created.";
		}
		else {
			m_activePoll = "http://" + STRAWPOLL_HOST + "/" + response["id"].asString();
			output += "Poll created : " + m_activePoll;
		}
	}
	else {
		output += "Error connecting to server.";
	}

	return output;
}

std::string CommandHandler::activeFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	return "Active poll: " + (m_activePoll.empty() ? "No poll has been created." : m_activePoll);
}

std::string CommandHandler::commandsFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	return "[COMMANDS] " + SOURCE + "/wiki/Default-Commands";
}

std::string CommandHandler::aboutFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	return "[ABOUT] " + m_name + " is running " + BOT_NAME + " " + BOT_VERSION + ". Find out more at " + SOURCE;
}

std::string CommandHandler::countFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() != 2 || !(tokens[1] == "start" || tokens[1] == "stop" || tokens[1] == "display")) {
		return "Invalid syntax.";
	}

	if (tokens[1] == "start") {
		/* begin a new count */
		if (m_counting) {
			return "A count is already running. End it before starting a new one.";
		}
		m_usersCounted.clear();
		m_messageCounts.clear();
		m_counting = true;
		return "Message counting has begun. Prepend your message with a + to have it counted.";
	}
	else if (tokens[1] == "stop") {
		/* end the current count */
		if (!m_counting) {
			return "There is no active count.";
		}
		m_counting = false;
		return "Count ended. Use \"$count display\" to view the results.";
	}
	else {
		/* display results from last count */
		if (m_counting) {
			return "End the count before viewing the results.";
		}
		if (m_messageCounts.empty()) {
			return "Nothing to display.";
		}
		else {
			typedef std::pair<std::string, uint16_t> mcount;
			// add each result to a vector to be sorted
			std::vector<mcount> pairs;
			for (auto itr = m_messageCounts.begin(); itr != m_messageCounts.end(); ++itr) {
				pairs.push_back(*itr);
			}
			std::sort(pairs.begin(), pairs.end(), [=](mcount &a, mcount &b) { return a.second > b.second; });
			std::string results = "[RESULTS] ";
			// only show top 10 results
			size_t max = pairs.size() > 10 ? 10 : pairs.size();
			for (size_t i = 0; i < max; ++i) {
				mcount &pair = pairs[i];
				// print rank and number of votes
				results += std::to_string(i + 1) + ". " + pair.first + " (" + std::to_string(pair.second) + ")" + (i == max - 1 ? "." : ", ");
			}
			return results;
		}
	}
}

std::string CommandHandler::whitelistFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() > 2) {
		return "Invalid syntax. Use \"$whitelist SITE\".";
	}
	if (tokens.size() == 1) {
		/* no args: show current whitelist */
		return m_modp->getFormattedWhitelist();
	}

	if (m_parsep->parse(tokens[1])) {
		/* extract website and add to whitelist */
		std::string website = m_parsep->getLast()->domain;
		m_modp->whitelist(website);
		return "@" + nick + ", " + website + " has been whitelisted.";
	}

	return "@" + nick + ", invalid URL: " + tokens[1];
}

std::string CommandHandler::permitFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() != 2) {
		return "Invalid syntax. Use \"$permit USER\".";
	}

	m_modp->permit(tokens[1]);
	return tokens[1] + " has been granted permission to post a link.";
}

std::string CommandHandler::makecomFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	if (!m_customCmds.isActive()) {
		return "Custom commands are currently disabled.";
	}

	bool editing = fullCmd.substr(0, 4) == "edit";
	bool success = true;
	std::string output = "@" + nick + ", ";
	time_t cooldown = editing ? -1 : 15;

	OptionParser op(fullCmd, "c:");
	int16_t c;
	while ((c = op.getopt()) != EOF) {
		switch (c) {
		case 'c':
			/* user provided a cooldown */
			try {
				cooldown = std::stoi(op.optarg());
				if (cooldown < 0) {
					return output + "cooldown cannot be negative.";
				}
			}
			catch (std::invalid_argument) {
				output += "invalid number: " + op.optarg();
				success = false;
			}
			break;
		case '?':
			/* invalid option */
			if (op.optopt() == 'c') {
				output += "no cooldown provided.";
			}
			else {
				return output + "illegal option: " + (char)op.optopt();
			}
			success = false;
			break;
		default:
			return "";
		}
	}

	if (success && op.optind() >= fullCmd.length()) {
		output += "not enough arguments given.";
		success = false;
	}
	if (!success)
		return output;

	std::string args = fullCmd.substr(op.optind());
	std::string::size_type sp = args.find(' ');
	if (editing) {
		// determine which parts are being changed
		bool changedCd = cooldown != -1;
		bool changedResp = sp != std::string::npos;
		const std::string cmd = changedResp ? args.substr(0, sp) : args;
		const std::string response = changedResp ? args.substr(sp + 1) : "";
		if (!m_customCmds.editCom(cmd, response, cooldown)) {
			output += "invalid command: $" + cmd;
		}
		else if (!changedCd && !changedResp) {
			output += "command $" + cmd + " was unchanged.";
		}
		else {
			// build an output string detailing changes
			output += "command $" + cmd + " has been changed to ";
			if (changedResp) {
				output += "\"" + response + "\"" + (changedCd ? ", with " : ".");
			}
			if (changedCd) {
				// reset cooldown in TimerManager
				m_cooldowns.remove(cmd);
				m_cooldowns.add(cmd, cooldown);
				output += "a " + std::to_string(cooldown) + "s cooldown.";
			}
		}
	}
	else {
		// adding a new command
		if (sp == std::string::npos) {
			output += "no response provided for command $" + args + ".";
		}
		else {
			// first word is command, rest is response
			std::string cmd = args.substr(0, sp);
			std::string response = args.substr(sp + 1);
			if (!m_customCmds.addCom(cmd, response, cooldown)) {
				output += "invalid command name: $" + cmd;
			}
			else {
				output += "command $" + cmd + " has been added with a " +
					std::to_string(cooldown) + "s cooldown";
			}
		}
	}
	return output;
}

std::string CommandHandler::delcomFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";
	if (!m_customCmds.isActive()) {
		return "Custom commands are currently disabled.";
	}

	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);

	if (tokens.size() == 2) {
		return "@" + nick + ", command $" + tokens[1] + (m_customCmds.delCom(tokens[1]) ? " has been deleted." : " not found.");
	}
	else {
		return "Invalid syntax. Use \"$delcom COMMAND\".";
	}
}

std::string CommandHandler::addrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	std::string output = "@" + nick + ", ";
	time_t cooldown = 300;
	int16_t c;
	OptionParser op(fullCmd, "c:");
	while ((c = op.getopt()) != EOF) {
		switch (c) {
		case 'c':
			try {
				// convert to seconds
				cooldown = 60 * std::stoi(op.optarg());
			}
			catch (std::invalid_argument) {
				return output + "invalid number: " + op.optarg();
			}
			break;
		case '?':
			if (op.optopt() == 'c') {
				return output + "no interval provided.";
			}
			return output + "illegal option: " + (char)op.optopt();
		default:
			return "";
		}
	}

	if (cooldown % 300 != 0) {
		return output + "cooldown must be a multiple of 5 minutes.";
	}
	else if (cooldown > 3600) {
		return output + "cooldown cannot be longer than an hour.";
	}
	if (op.optind() >= fullCmd.length()) {
		output += "no message specified.";
	}
	else if (!m_evtp->addMessage(fullCmd.substr(op.optind()), cooldown)) {
		output += "limit of 5 recurring messages reached.";
	}
	else {
		output += "recurring message \"" + fullCmd.substr(op.optind()) + "\" has been added at a "
			+ std::to_string(cooldown / 60) + " min interval.";
	}
	return output;
}

std::string CommandHandler::delrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";

	std::string output = "@" + nick + ", ";
	std::vector<std::string> tokens;
	utils::split(fullCmd, ' ', tokens);
	if (tokens.size() != 2) {
		return output += "invalid syntax. Use \"$delrec ID\".";
	}

	uint32_t id;
	try {
		id = std::stoi(tokens[1]);
	}
	catch (std::invalid_argument) {
		return output += "invalid number: " + tokens[1];
	}
	if (!m_evtp->delMessage(id)) {
		output += "invalid ID provided. Use \"$listrec\" to show all recurring message IDs.";
	}
	else {
		output += "recurring message " + std::to_string(id) + " deleted.";
	}
	return output;
}

std::string CommandHandler::listrecFunc(const std::string &nick, const std::string &fullCmd, bool privileges)
{
	if (!privileges)
		return "";
	return m_evtp->messageList();
}

std::string CommandHandler::extractCMLData(const std::string &httpResp, const std::string &rsn) const
{
	std::vector<std::string> elems;
	utils::split(httpResp, ',', elems);
	if (elems.size() == 4) {
		std::string ehp = elems[2];
		std::string::size_type dot;
		if ((dot = ehp.find(".")) != std::string::npos) {
			// truncate to one decimal place
			ehp = ehp.substr(0, dot + 2);
		}
		return "Name: " + elems[1] + ", Rank: " + elems[0] + ", EHP: " + ehp + " (+" + elems[3] + " this week).";
	}
	else {
		return "Player either does not exist or has not been tracked on CML.";
	}
}

std::string CommandHandler::extractHSData(const std::string &httpResp, uint8_t skillID) const
{
	std::vector<std::string> skills;
	utils::split(httpResp, '\n', skills);
	
	std::vector<std::string> tokens;
	utils::split(skills[skillID], ',', tokens);

	return "Level: " + tokens[1] + ", Exp: " + utils::formatInteger(tokens[2]) + ", Rank: " + utils::formatInteger(tokens[0]) + ".";
}

std::string CommandHandler::extractGEData(const std::string &httpResp) const
{
	Json::Reader reader;
	Json::Value item;
	if (reader.parse(httpResp, item)) {
		return utils::formatInteger(item["overall"].asString());
	}
	else {
		return "An error occurred. Please try again.";
	}
}
