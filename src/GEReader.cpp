#include "stdafx.h"

GEReader::GEReader() {

	readItemIDs();

}

GEReader::~GEReader() {};

void GEReader::readItemIDs() {

	Json::Reader reader;
	std::ifstream ids(getApplicationDirectory() + "\\itemids.json", std::ifstream::binary);

	if (!reader.parse(ids, m_itemIDs)) {
		std::cerr << reader.getFormattedErrorMessages() << std::endl;
	}

	std::clog << "Successfully read RS item IDs." << std::endl;


}

Json::Value GEReader::getItem(std::string &name) {

	std::transform(name.begin(), name.end(), name.begin(), tolower);
	name[0] = toupper(name[0]);

	for (auto &val : m_itemIDs["items"]) {
		
		if (val["name"].asString() == name) {
			return val;
		}

		if (val.isMember("nick")) {
			for (auto &nickname : val["nick"]) {
				if (nickname.asString() == name) {
					return val;
				}
			}
		}

	}

	// return empty value if item not found
	return Json::Value();

}

std::string GEReader::extractItemPrice(const std::string &itemJson) {

	Json::Reader reader;
	Json::Value itemPrices;
	if (!reader.parse(itemJson, itemPrices)) {
		std::cerr << reader.getFormattedErrorMessages();
		return "An error occurred. Please try again.";
	}
	return formatInteger(itemPrices["overall"].asString());

}