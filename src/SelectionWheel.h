#pragma once

#include <ctime>
#include <string>
#include <map>
#include <json/json.h>

typedef std::map<std::string, std::pair<std::string, time_t>> wheel_map;

class SelectionWheel {

	public:
		SelectionWheel();
		~SelectionWheel();
		bool active();
		const char *name() const;
		const char *cmd() const;
		const char *desc() const;
		const char *usage() const;
		bool valid(const char *category) const;
		const char *choose(const char *nick, const char *category);
		bool ready(const char *nick) const;
		const char *selection(const char *nick) const;

	private:
		Json::Value wheeldata;
		wheel_map stored;
		time_t cooldown;
		bool enabled;
		void add(const std::string &nick, const std::string &selection);
		time_t lastUsed(const std::string &nick) const;

};
