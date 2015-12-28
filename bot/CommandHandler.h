#pragma once

class CommandHandler {

	public:
		CommandHandler();
		~CommandHandler();
		std::string processCommand(const std::string &nick, const std::string &fullCmd);
	private:
		const std::string CML_HOST = "www.crystalmathlabs.com";
		const std::string CML_EHP_AHI = "/tracker/api.php?type=virtualhiscoresatplayer&page=timeplayed&player=";
		std::string handleCalc(const std::string &fullCmd);

};