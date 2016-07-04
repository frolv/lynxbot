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
		bool delMessage(size_t id);
		std::string messageList() const;
		std::string message(size_t id, int lim = -1) const;
		std::vector<std::pair<std::string, time_t>> *messages();
		time_t init();
	private:
		time_t m_init;
		ConfigReader *m_cfgr;
		bool m_msg;
		std::vector<std::pair<std::string, time_t>> m_messages;
		void readFile();
		void writeFile();
		void reloadMessages();

};
