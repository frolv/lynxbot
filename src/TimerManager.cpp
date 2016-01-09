#include "stdafx.h"

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::add(const std::string &name, std::time_t cooldown) {

	std::pair<std::time_t, std::time_t> cmdTimes = { cooldown, 0 };
	TimerMap::value_type val = { name, cmdTimes };
	m_timers.insert(val);

}

bool TimerManager::ready(const std::string &cmd) {
	return std::time(nullptr) - lastUsed(cmd) >= cooldown(cmd);
}

void TimerManager::setUsed(const std::string &cmd) {
	m_timers.find(cmd)->second.second = std::time(nullptr);
}

std::time_t TimerManager::cooldown(const std::string &cmd) {
	return m_timers.find(cmd)->second.first;
}

std::time_t TimerManager::lastUsed(const std::string &cmd) {
	return m_timers.find(cmd)->second.second;
}