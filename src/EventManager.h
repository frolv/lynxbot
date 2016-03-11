#pragma once

#include <string>
#include <vector>
#include "TimerManager.h"

class TimerManager;

class EventManager : public TimerManager {

	public:
		EventManager();
		~EventManager();
		bool addMessage(const std::string &message, time_t cooldown);
		bool delMessage(uint32_t id);
		std::string messageList() const;
		std::vector<std::pair<std::string, time_t>> *messages();
	private:
		std::vector<std::pair<std::string, time_t>> m_messages;
		void readFile();
		void writeFile();

};