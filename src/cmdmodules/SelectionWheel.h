#pragma once

typedef std::map<std::string, std::pair<std::string, std::time_t>> WheelMap;

class SelectionWheel {

	public:
		SelectionWheel();
		~SelectionWheel();
		inline bool isActive() { return m_active; };
		std::string name();
		std::string cmd();
		std::string desc();
		std::string usage();
		bool valid(const std::string &category);
		std::string choose(const std::string &nick, const std::string &category);
		bool ready(const std::string &nick);
		std::string selection(const std::string &nick);

	private:
		Json::Value m_data;
		WheelMap m_stored;
		bool m_active;
		void add(const std::string &nick, const std::string &selection);
		std::time_t lastUsed(const std::string &nick);

};