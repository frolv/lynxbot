#pragma once

#include <string>
#include "config.h"

class ConfigReader;

class Giveaway {
	public:
		Giveaway(const std::string &channel, time_t initTime,
				ConfigReader *cfgr);
		~Giveaway();
		bool init(time_t initTime, bool first);
		bool active() const;
		bool activate(time_t initTime, std::string &reason);
		void setFollowers(bool setting, uint32_t amt = 0);
		void setTimer(bool setting, time_t interval = 0);
		void deactivate();
		/* bool checkSubs(); */
		bool checkConditions(time_t curr);
		std::string giveaway();
		uint32_t followers();
		time_t interval();
		std::string currentSettings(int8_t type = -1);
	private:
		ConfigReader *m_cfgr;
		bool m_active;
		bool m_type[3] = { false, false, false };
		const std::string m_channel;
		uint8_t m_reason;
		uint32_t m_followerLimit;
		uint32_t m_currFollowers;
		time_t m_lastRequest;
		time_t m_interval;
		time_t m_earliest;
		time_t m_latest;
		std::vector<std::string> m_items;
		bool initialize();
		uint32_t getFollowers() const;
		void interactiveFollowers();
		bool readSettings();
		bool readGiveaway();
		void writeGiveaway() const;
		void writeSettings() const;
		void updateTimes(time_t curr);
		std::string getItem();
};
