#pragma once

#include <string>
#include <vector>
#include "config.h"
#include "TimerManager.h"

class ConfigReader;
class TimerManager;

class EventManager : public TimerManager {

	public:
		EventManager(ConfigReader *cfgr);
		~EventManager();
		bool messagesActive();
		void activateMessages();
		void deactivateMessages();
		bool addMessage(const std::string &message,
				time_t cooldown, bool write = true);
		bool delMessage(uint32_t id);
		std::string messageList() const;
		std::vector<std::pair<std::string, time_t>> *messages();
	private:
		ConfigReader *m_cfgr;
		bool m_msg;
		std::vector<std::pair<std::string, time_t>> m_messages;
		void readFile();
		void writeFile();
		void reloadMessages();

};
