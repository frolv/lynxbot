#include "TimerManager.h"

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::add(const std::string &name,
		time_t cooldown, time_t lastUsed)
{
	std::pair<time_t, time_t> cmdtimes = { cooldown, lastUsed };
	timer_map::value_type val = { name, cmdtimes };
	timers.insert(val);
}

void TimerManager::remove(const std::string &name)
{
	timers.erase(name);
}

bool TimerManager::ready(const std::string &cmd) const
{
	if (timers.find(cmd) == timers.end())
		return false;

	return time(nullptr) - lastUsed(cmd) >= cooldown(cmd);
}

void TimerManager::set_used(const std::string &cmd)
{
	timers.find(cmd)->second.second = time(nullptr);
}
void TimerManager::clear()
{
	timers.clear();
}

time_t TimerManager::cooldown(const std::string &cmd) const
{
	return timers.find(cmd)->second.first;
}

time_t TimerManager::lastUsed(const std::string &cmd) const
{
	return timers.find(cmd)->second.second;
}
