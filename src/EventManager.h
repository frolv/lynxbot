#pragma once

#include <string>
#include <vector>
#include "TimerManager.h"

class TimerManager;

class EventManager : public TimerManager {

	public:
		EventManager();
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
		bool m_msg;
		std::vector<std::pair<std::string, time_t>> m_messages;
		void readFile();
		void writeFile();
		void reloadMessages();

};
