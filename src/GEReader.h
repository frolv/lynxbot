#pragma once

#include <json/json.h>

class GEReader {

	public:
		GEReader();
		~GEReader();
		bool active() const;
		Json::Value getItem(std::string &name) const;
	private:
		Json::Value m_itemIDs;
		bool m_active;

};
