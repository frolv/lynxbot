#pragma once

#include "TimerManager.h"
#include "CommandHandler.h"

class TimerManager;
class CommandHandler;

typedef std::map<std::string, std::string(CommandHandler::*)(const std::string &, const std::string &, bool)> commandMap;

class CustomCommandHandler {

	public:
		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm, const std::string &wheelCmd);
		~CustomCommandHandler();
		bool isActive();
		void addCom(const std::string &cmd, const std::string &response, std::time_t cooldown);
		bool delCom(const std::string &cmd);
		Json::Value getCom(const std::string &cmd);
		void writeToFile();
		bool validName(const std::string &cmd, bool loading = false);

	private:
		bool m_active;
		commandMap *m_cmp;
		TimerManager *m_tmp;
		const std::string m_wheelCmd;
		Json::Value m_commands;

};