#include "stdafx.h"

GEReader::GEReader() {

	m_active = utils::readJSON("itemids.json", m_itemIDs);
	if (!m_active) {
		std::cerr << "Failed to read RS Item IDs. $ge command will be disabled for this session." << std::endl;
	}

}

GEReader::~GEReader() {};

bool GEReader::active() const {
	return m_active;
}

Json::Value GEReader::getItem(std::string &name) const {

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