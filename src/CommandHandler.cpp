#include "stdafx.h"

// will be changed when custom commands are added
CommandHandler::CommandHandler() {}

CommandHandler::~CommandHandler() {}

std::string CommandHandler::processCommand(const std::string &nick, const std::string &fullCmd) {

	std::vector<std::string> tokens;
	split(fullCmd, ' ', tokens);
	std::string output, cmd = tokens[0];

	if (cmd == "ehp") {

		if (tokens.size() == 2) {
			// a username was provided
			std::string rsn = tokens[1];
			std::replace(rsn.begin(), rsn.end(), '-', '_');
			const std::string httpResp = HTTPReq(CML_HOST, CML_EHP_AHI + rsn);
			std::clog << httpResp << std::endl << std::endl;
			output = "[EHP] " + extractCMLData(httpResp, rsn);

		}
		else if (tokens.size() == 1) {
			output = "EHP stands for efficient hours played. You earn 1 EHP whenever you gain a certain amount of experience \
				in a skill, depending on your level. You can find XP rates here: http://crystalmathlabs.com/tracker/suppliescalc.php";
		}
		else {
			output = "Invalid syntax. Use \"$ehp [RSN]\".";
		}
	}
	else if (cmd == "level" || cmd == "lvl") {
	
		if (tokens.size() == 3) {

			if (skillMap.find(tokens[1]) == skillMap.end() && skillNickMap.find(tokens[1]) == skillNickMap.end()) {
				return "Invalid skill name.";
			}
			uint8_t skillID = skillMap.find(tokens[1]) == skillMap.end() ? skillNickMap.find(tokens[1])->second : skillMap.find(tokens[1])->second;
			
			std::string rsn = tokens[2];
			std::replace(rsn.begin(), rsn.end(), '-', '_');

			const std::string httpResp = HTTPReq(RS_HOST, RS_HS_API + rsn);
			std::clog << httpResp << std::endl;
			if (httpResp.find("404 - Page not found") != std::string::npos) {
				return "Player not found on hiscores.";
			}

			std::string nick = getSkillNick(skillID);
			std::transform(nick.begin(), nick.end(), nick.begin(), ::toupper);

			output = "[" + nick + "] Name: " + rsn + ", " + extractHSData(httpResp, skillID);
		}
		else {
			output = "Invalid syntax. Use \"$lvl SKILL RSN\".";
		}

	}
	else if (cmd == "ge") {

		std::string itemName = fullCmd.substr(3);
		std::replace(itemName.begin(), itemName.end(), '_', ' ');

		Json::Value item = m_GEReader.getItem(itemName);

		if (item.empty()) {
			return "Item not found: " + itemName;
		}

		const std::string httpResp = HTTPReq(EXCHANGE_HOST, EXCHANGE_API + std::to_string(item["id"].asInt()));
		std::clog << httpResp;

		output = "[GE] " + item["name"].asString() + ": " + extractGEData(httpResp) + " gp.";

	}
	else if (cmd == "calc") {
		try {
			output = "[CALC] " + handleCalc(fullCmd);
		}
		catch (std::runtime_error &e) {
			output = e.what();
		}
	}
	else if (cmd == "cml") {
		output = "[CML] http://" + CML_HOST;
	}
	else {
		output = "Invalid command";
		std::cerr << output << ": " << cmd << std::endl << std::endl;
	}

	return output;

}

std::string CommandHandler::handleCalc(const std::string &fullCmd) {

	if (fullCmd.length() < 6) {
		throw std::runtime_error("Invalid mathematical expression.");
	}

	std::string expr = fullCmd.substr(5);
	// remove all whitespace
	expr.erase(std::remove_if(expr.begin(), expr.end(), isspace), expr.end());
	
	std::ostringstream result;

	ExpressionParser exprP(expr);
	exprP.tokenizeExpr();
	double res = exprP.eval();
	result << res;

	std::string resultStr = result.str();

	if (resultStr == "inf" || resultStr == "-nan(ind)") {
		resultStr = "Error: division by 0.";
	}

	return resultStr;

}

std::string CommandHandler::extractCMLData(const std::string &httpResp, const std::string &rsn) {

	std::regex dataRegex("(\\d+," + rsn + ",[\\d\\.]+,[\\d\\.]+)", std::regex_constants::ECMAScript | std::regex_constants::icase);
	std::smatch match;
	// find the required data
	if (std::regex_search(httpResp.begin(), httpResp.end(), match, dataRegex)) {

		std::string data = match[1];
		std::vector<std::string> elems;
		// split data into tokens
		split(data, ',', elems);
		std::string ehp = elems[2];

		if (ehp.find(".") != std::string::npos) {
			// truncate to one decimal place
			ehp = ehp.substr(0, ehp.find(".") + 2);
		}
		return "Name: " + elems[1] + ", Rank: " + elems[0] + ", EHP: " + ehp + " (+" + elems[3] + " this week).";

	}
	else {
		return "Player either does not exist or has not been tracked on CML.";
	}

}

std::string CommandHandler::extractHSData(const std::string &httpResp, uint8_t skillID) {

	std::vector<std::string> skills;
	split(httpResp, '\n', skills);
	
	std::vector<std::string> tokens;
	split(skills[skillID], ',', tokens);

	return "Level: " + tokens[1] + ", Exp: " + formatInteger(tokens[2]) + ", Rank: " + formatInteger(tokens[0]) + ".";

}

std::string CommandHandler::extractGEData(const std::string &httpResp) {

	std::regex jsonRegex("(\\{.+\\})");
	std::smatch match;

	if (std::regex_search(httpResp.begin(), httpResp.end(), match, jsonRegex)) {
		const std::string json = match[1];
		return m_GEReader.extractItemPrice(json);
	}
	else {
		return "An error occured. Please try again.";
	}

}