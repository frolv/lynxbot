#pragma once

#define DEFAULT_COOLDOWN 15

typedef std::map<std::string, std::pair<std::time_t, std::time_t>> TimerMap;

class TimerManager {

	public:
		TimerManager();
		~TimerManager();
		void add(const std::string &name, std::time_t cooldown = DEFAULT_COOLDOWN);
		bool ready(const std::string &cmd) const;
		void setUsed(const std::string &cmd);
		
	private:
		TimerMap m_timers;
		std::time_t cooldown(const std::string &cmd) const;
		std::time_t lastUsed(const std::string &cmd) const;

};