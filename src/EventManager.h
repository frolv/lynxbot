#pragma once

#include <string>
#include <vector>
#include "config.h"
#include "TimerManager.h"

class ConfigReader;
class TimerManager;

class EventManager : public TimerManager {

	public:
		EventManager(ConfigReader *cfgr);
		~EventManager();
		bool active();
		void activate();
		void deactivate();
		bool addmsg(const char *msg, time_t cd, bool write = true);
		bool delmsg(size_t id);
		bool editmsg(size_t id, const char *msg, time_t cd);
		std::string msglist() const;
		std::string message(size_t id, int lim = -1) const;
		std::vector<std::pair<std::string, time_t>> *messages();
	private:
		ConfigReader *m_cfgr;
		bool m_msg;
		std::vector<std::pair<std::string, time_t>> m_messages;
		void read_messages();
		void write_messages();
		void reload_messages();

};
