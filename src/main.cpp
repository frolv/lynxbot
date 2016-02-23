#include "stdafx.h"
#include "TwitchBot.h"
#include <pencode.h>

struct botData {
	std::string name;
	std::string channel;
	std::string pass;
};

botData readSettings(const std::string &appDir);

int main() {

	botData bd;

	try {
		bd = readSettings(utils::getApplicationDirectory());
		if (bd.name.empty()) {
			throw std::runtime_error("Could not extract name from settings.txt");
		}
		if (bd.channel.empty()) {
			throw std::runtime_error("Could not extract channel from settings.txt");
		}
		if (bd.pass.empty()) {
			throw std::runtime_error("Could not extract password from settings.txt");
		}
	}
	catch (std::runtime_error &e) {
		std::cerr << e.what();
		std::cin.get();
		return 1;
	}

	TwitchBot bot(bd.name, bd.channel, bd.pass);

	if (bot.isConnected()) {
		bot.serverLoop();
	}

	return 0;

}

botData readSettings(const std::string &appDir) {

	// open settings.cfg
	std::ifstream reader(appDir + "\\settings.txt");
	if (!reader.is_open()) {
		throw std::runtime_error("Could not locate settings.txt");
	}

	std::string line;
	uint8_t lineNum = 0;
	botData bd;

	while (std::getline(reader, line)) {

		lineNum++;
		// remove whitespace
		line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

		// lines starting with * are comments
		if (utils::startsWith(line, "*")) {
			continue;
		}
		else {
			std::regex lineRegex("(name|channel|password):(.+?)");
			std::smatch match;
			const std::string s = line;
			if (std::regex_match(s.begin(), s.end(), match, lineRegex)) {
				if (match[1].str() == "name") {
					bd.name = match[2];
				}
				else if (match[1].str() == "channel") {
					if (!utils::startsWith(match[2].str(), "#")) {
						throw std::runtime_error("Channel name must start with #.");
					}
					bd.channel = match[2];
				}
				else {
					if (!utils::startsWith(match[2].str(), "oauth:")) {
						throw std::runtime_error("Password must be a valid oauth token, starting with \"oauth:\".");
					}
					bd.pass = match[2];
				}
			}
			else {
				throw std::runtime_error("Syntax error on line " + std::to_string(lineNum) + " of settings.txt.");
			}
		}

	}

	reader.close();
	return bd;

}