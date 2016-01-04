#pragma once

class TimerManager {

	public:
		TimerManager();
		~TimerManager();
		bool ready(const std::string &cmd);
		void setUsed(const std::string &cmd);

	private:
		std::map<std::string, std::time_t> m_cooldowns;
		std::map<std::string, std::time_t> m_lastUsed;
		std::time_t cooldown(const std::string &cmd);
		std::time_t lastUsed(const std::string &cmd);

};