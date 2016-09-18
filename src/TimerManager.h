#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <ctime>
#include <unordered_map>

#define DEFAULT_COOLDOWN 10

typedef std::unordered_map<std::string, std::pair<time_t, time_t>> timer_map;

class TimerManager {

	public:
		TimerManager();
		~TimerManager();
		void add(const std::string &name,
			time_t cooldown = DEFAULT_COOLDOWN, time_t last_used = 0);
		void remove(const std::string &name);
		bool ready(const std::string &cmd) const;
		void set_used(const std::string &cmd);
		void clear();

	private:
		timer_map timers;
		time_t cooldown(const std::string &cmd) const;
		time_t lastUsed(const std::string &cmd) const;

};

#endif
