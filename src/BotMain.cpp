#include "stdafx.h"

int main() {

	std::vector<std::string> settings;

	try {
		settings = readSettings(getApplicationDirectory());
		if (settings.size() != 3) {
			throw std::runtime_error("settings.txt is improperly configured.");
		}
	}
	catch (std::runtime_error &e) {
		std::cerr << e.what();
		std::cin.get();
		return 1;
	}

	TwitchBot bot(settings[0], settings[1], settings[2]);

	if (bot.isConnected()) {
		bot.serverLoop();
	}

	return 0;

}