#pragma once

class CommandHandler {

	public:
		CommandHandler();
		~CommandHandler();
		std::string processCommand(const std::string &nick, const std::string &fullCmd);

	private:

};