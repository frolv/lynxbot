#include "stdafx.h"

int main() {

	std::vector<std::string> config;

	try {
		config = readCFG();
		if (config.size() != 3) {
			throw std::runtime_error("settings.cfg is improperly configured.");
		}
	}
	catch (std::runtime_error &e) {
		std::cerr << e.what();
		std::cin.get();
		return 1;
	}

	TwitchBot bot(config[0], config[1], config[2]);

	if (bot.isConnected()) {
		bot.serverLoop();
	}

	return 0;

}