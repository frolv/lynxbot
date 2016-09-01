#include "TimerManager.h"

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::add(const std::string &name,
		time_t cooldown, time_t lastUsed)
{
	std::pair<time_t, time_t> cmdTimes = { cooldown, lastUsed };
	TimerMap::value_type val = { name, cmdTimes };
	m_timers.insert(val);
}

void TimerManager::remove(const std::string &name)
{
	m_timers.erase(name);
}

bool TimerManager::ready(const std::string &cmd) const
{
	if (m_timers.find(cmd) == m_timers.end())
		return false;

	return time(nullptr) - lastUsed(cmd) >= cooldown(cmd);
}

void TimerManager::setUsed(const std::string &cmd)
{
	m_timers.find(cmd)->second.second = time(nullptr);
}
void TimerManager::clear()
{
	m_timers.clear();
}

time_t TimerManager::cooldown(const std::string &cmd) const
{
	return m_timers.find(cmd)->second.first;
}

time_t TimerManager::lastUsed(const std::string &cmd) const
{
	return m_timers.find(cmd)->second.second;
}
