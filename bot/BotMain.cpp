#include "stdafx.h"
#include "TwitchBot.h"

int main() {

	TwitchBot bot("bot_test_", "irc.twitch.tv", "6667", "#brainsoldier", "oauth:swoqkwa5cagrf5qbvmpvqbgua5e3ia");

	// temp to prevent closing
	std::cin.get();

	return 0;

}