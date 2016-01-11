#pragma once

class GEReader {

	public:
		GEReader();
		~GEReader();
		bool active();
		Json::Value getItem(std::string &name);
		std::string extractItemPrice(const std::string &itemJson);
	private:
		Json::Value m_itemIDs;
		bool m_active;

};