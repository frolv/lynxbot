#pragma once

#include <unordered_map>
#include <json/json.h>
#include "TimerManager.h"
#include "CommandHandler.h"

class TimerManager;
class CommandHandler;

class CustomCommandHandler {

	public:
		typedef std::unordered_map<std::string,
			std::string(CommandHandler::*)(CommandHandler::cmdinfo *)> commandMap;
		CustomCommandHandler(commandMap *defaultCmds, TimerManager *tm, const std::string &wheelCmd);
		~CustomCommandHandler();
		bool isActive();
		bool addCom(const std::string &cmd, const std::string &response, std::time_t cooldown);
		bool delCom(const std::string &cmd);
		bool editCom(const std::string &cmd, const std::string &newResp = "", std::time_t newcd = -1);
		Json::Value *getCom(const std::string &cmd);
		bool validName(const std::string &cmd, bool loading = false);

	private:
		bool m_active;
		commandMap *m_cmp;
		TimerManager *m_tmp;
		const std::string m_wheelCmd;
		Json::Value m_commands;
		Json::Value m_emptyVal;
		void writeToFile() const;

};
