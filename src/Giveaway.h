#pragma once

class Giveaway {
	public:
		Giveaway(const std::string &channel, time_t initTime);
		~Giveaway();
		bool active() const;
		bool checkSubs();
		bool checkConditions(time_t curr);
		std::string getItem();
	private:
		bool m_active;
		bool m_type[3] = { false, false, false };
		const std::string m_channel;
		time_t m_lastRequest;
		time_t m_interval;
		time_t m_earliest;
		time_t m_latest;
		uint16_t m_followerLimit;
		uint32_t m_currFollowers;
		std::vector<std::string> m_items;
		uint32_t getFollowers() const;
		bool readSettings();
		bool readGiveaway();
		void writeGiveaway() const;
		bool parseBool(const std::string &s) const;
		void updateTimes(time_t curr);
};