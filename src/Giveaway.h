#pragma once

class Giveaway {
	public:
		Giveaway();
		~Giveaway();
	private:
		bool m_active;
		bool m_type[3] = { false, false, false };
		time_t m_interval;
		uint16_t m_followerLimit;
		std::vector<std::string> m_items;
		bool readSettings();
		bool readGiveaway();
		bool parseBool(const std::string &s);
};