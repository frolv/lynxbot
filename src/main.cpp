#include <string>
#include <regex>
#include <fstream>
#include "TwitchBot.h"
#include "utils.h"

/* botData stores settings for initializing a TwitchBot instance */
struct botData {
	std::string name;
	std::string channel;
	std::string pass;
};

bool readSettings(botData *bd, std::string &error);

int main(int argc, char **argv)
{
	botData bd;
	std::string error;

	if (argc > 2) {
		std::cerr << "usage: " << argv[0] << " [CHANNEL]" << std::endl;
		return 1;
	}

	if (!readSettings(&bd, error)) {
		std::cerr << error << std::endl;
		std::cin.get();
		return 1;
	}

	std::string channel = bd.channel;
	if (argc == 2) {
		channel = argv[1];
		if (channel[0] != '#')
			channel = '#' + channel;
	}

	TwitchBot bot(bd.name, channel, bd.pass);

	if (bot.isConnected())
		bot.serverLoop();

	return 0;
}

bool readSettings(botData *bd, std::string &error)
{
	/* open settings */
	std::string path = utils::configdir() + utils::config("settings");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		error = "could not locate " + path;
		return false;
	}

	std::string line;
	uint8_t lineNum = 0;

	while (std::getline(reader, line)) {
		++lineNum;
		/* remove whitespace */
		line.erase(std::remove_if(line.begin(), line.end(), isspace),
				line.end());

		/* lines starting with * are comments */
		if (utils::startsWith(line, "*"))
			continue;

		std::regex lineRegex("(name|channel|password):(.+?)");
		std::smatch match;
		const std::string s = line;
		if (std::regex_match(s.begin(), s.end(), match, lineRegex)) {
			if (match[1].str() == "name") {
				bd->name = match[2];
			} else if (match[1].str() == "channel") {
				if (!utils::startsWith(match[2].str(), "#"))
					error = "Channel must start with #";
				bd->channel = match[2];
			} else {
				if (!utils::startsWith(match[2].str(), "oauth:"))
					error = "Password must be a valid "
						 "oauth token, starting with "
						 "\"oauth:\".";
				bd->pass = match[2];
			}
		} else {
			error = "Syntax error on line "
				+ std::to_string(lineNum) + " of " + path;
		}
		if (!error.empty())
			return false;
	}

	reader.close();
	return true;
}
