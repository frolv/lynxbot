#pragma once

typedef std::map<std::string, std::pair<std::time_t, std::time_t>> TimerMap;

class TimerManager {

	public:
		TimerManager();
		~TimerManager();
		void add(const std::string &name, std::time_t cooldown);
		bool ready(const std::string &cmd);
		void setUsed(const std::string &cmd);
		
	private:
		TimerMap m_timers;
		std::time_t cooldown(const std::string &cmd);
		std::time_t lastUsed(const std::string &cmd);

};