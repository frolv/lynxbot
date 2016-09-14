#pragma once

#include <json/json.h>
#include <string>
#include <unordered_map>

class GEReader {
	public:
		GEReader();
		~GEReader();
		bool active() const;
		const Json::Value *find_item(const char *name);
	private:
		std::unordered_map<std::string, std::string> nickmap;
		Json::Value items;
		bool enabled;
		void read_nicks();
		void sort();
};
