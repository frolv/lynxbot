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
			output = "[EHP] " + tokens[1] + ": 0 (+0.0 this week).";
		}
		else if (tokens.size() == 1) {
			output = "EHP stands for efficient hours played. Learn more: ";
		}
		else {
			output = "Invalid syntax. Use \"$ehp [RSN]\".";
		}
	}
	else if (cmd == "calc") {
		try {
			output = handleCalc(fullCmd);
		}
		catch (std::runtime_error &e) {
			output = e.what();
		}
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

	return result.str();

}