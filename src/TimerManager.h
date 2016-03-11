#pragma once

#include <unordered_map>

#define DEFAULT_COOLDOWN 15

typedef std::unordered_map<std::string, std::pair<time_t, time_t>> TimerMap;

class TimerManager {

	public:
		TimerManager();
		~TimerManager();
		void add(const std::string &name, time_t cooldown = DEFAULT_COOLDOWN, time_t lastUsed = 0);
		void remove(const std::string &name);
		bool ready(const std::string &cmd) const;
		void setUsed(const std::string &cmd);
		void clear();
		
	private:
		TimerMap m_timers;
		time_t cooldown(const std::string &cmd) const;
		time_t lastUsed(const std::string &cmd) const;

};