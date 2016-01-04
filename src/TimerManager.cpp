#include "stdafx.h"

TimerManager::TimerManager() {

	m_cooldowns = {
		{ "not enough", 15 }, { "imbue", 20 }
	};
	m_lastUsed = {
		{ "not enough", 0 }, { "imbue", 0 }
	};

}

TimerManager::~TimerManager() {}

bool TimerManager::ready(const std::string &cmd) {
		return std::time(nullptr) - lastUsed(cmd) >= cooldown(cmd);
}

void TimerManager::setUsed(const std::string &cmd) {
	m_lastUsed.find(cmd)->second = std::time(nullptr);
}

std::time_t TimerManager::cooldown(const std::string &cmd) {
	return m_cooldowns.find(cmd)->second;
}

std::time_t TimerManager::lastUsed(const std::string &cmd) {
	return m_lastUsed.find(cmd)->second;
}