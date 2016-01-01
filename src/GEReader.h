#pragma once

class GEReader {

	public:
		GEReader();
		~GEReader();
		Json::Value getItem(std::string &name);
		std::string extractItemPrice(const std::string &itemJson);
	private:
		Json::Value m_itemIDs;
		void readItemIDs();

};