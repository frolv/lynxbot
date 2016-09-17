#pragma once

#include <string>
#include "config.h"

class ConfigReader;

class Giveaway {
	public:
		Giveaway(const char *channel, time_t init_time,
				ConfigReader *cfgr);
		~Giveaway();
		bool init(time_t init_time, bool first);
		bool active() const;
		bool activate(time_t init_time);
		void set_followers(bool setting, uint32_t amt = 0);
		void set_timer(bool setting, time_t interval = 0);
		void set_images(bool setting);
		void deactivate();
		/* bool check_subs(); */
		bool check(time_t curr);
		std::string giveaway();
		uint32_t followers();
		time_t interval();
		std::string current_settings(int8_t type = -1);
		char *err();
	private:
		ConfigReader *cfg;
		bool enabled;
		bool givtype[3] = { false, false, false };
		bool images;
		const char *bot_channel;
		uint8_t reason;
		uint32_t follower_limit;
		uint32_t curr_followers;
		time_t last_check;
		time_t timed_interval;
		time_t earliest;
		time_t latest;
		std::vector<std::string> items;
		char error[256];
		bool initialize();
		uint32_t get_followers() const;
		void interactive_followers();
		bool read_settings();
		bool read_giveaway();
		void write_giveaway() const;
		void write_settings() const;
		void update_times(time_t curr);
		std::string get_item();
};
