#pragma once

#include <json/json.h>
#include <string>
#include <unordered_map>

class GEReader {

	public:
		GEReader();
		~GEReader();
		bool active() const;
		Json::Value getItem(std::string &name) const;
	private:
		std::unordered_map<std::string, std::string> m_nicks;
		Json::Value m_itemIDs;
		bool m_active;
		void readNicks();
		void sort();

};
