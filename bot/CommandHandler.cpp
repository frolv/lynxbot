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
	else {
		output = "Invalid command";
		std::cerr << output << ": " << cmd << std::endl << std::endl;
	}

	return output;

}