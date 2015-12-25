#include "stdafx.h"
#include "TwitchBot.h"

int main() {

	TwitchBot bot("name", "user", "irc.twitch.tv", "6667", "channel", "password");

	// temp to prevent closing
	std::cin.get();

	return 0;

}