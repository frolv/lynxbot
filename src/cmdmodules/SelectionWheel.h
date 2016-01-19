#pragma once

typedef std::map<std::string, std::pair<std::string, std::time_t>> WheelMap;

class SelectionWheel {

	public:
		SelectionWheel();
		~SelectionWheel();
		bool isActive();
		std::string name() const;
		std::string cmd() const;
		std::string desc() const;
		std::string usage() const;
		bool valid(const std::string &category) const;
		std::string choose(const std::string &nick, const std::string &category);
		bool ready(const std::string &nick) const;
		std::string selection(const std::string &nick) const;

	private:
		Json::Value m_data;
		WheelMap m_stored;
		bool m_active;
		void add(const std::string &nick, const std::string &selection);
		std::time_t lastUsed(const std::string &nick) const;

};