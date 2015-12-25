#include "stdafx.h"
#include "TwitchBot.h"

int main() {

	TwitchBot bot("bot_test_", "#brainsoldier", "oauth:heri34fz0206epagbry8acezxt8dst");

	if (bot.isConnected()) {
		bot.serverLoop();
	}

	return 0;

}