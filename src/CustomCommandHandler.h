#pragma once

#include "TimerManager.h"
#include "CommandHandler.h"

class TimerManager;
class CommandHandler;

typedef std::map<std::string, std::string(CommandHandler::*)(const std::string &, const std::string &)> commandMap;

class CustomCommandHandler {

	public:
		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm);
		~CustomCommandHandler();
		bool isActive();
		void addCom(const std::string &cmd, const std::string &response, std::time_t cooldown);
		void delCom(const std::string &cmd);
		Json::Value getCom(const std::string &cmd);
		void writeToFile();
		bool validName(const std::string &cmd);

	private:
		bool m_active;
		commandMap *m_cmp;
		TimerManager *m_tmp;
		Json::Value m_commands;

};