#include <algorithm>
#include <iostream>
#include "GEReader.h"
#include "utils.h"

GEReader::GEReader()
{
	if (!(m_active = utils::readJSON("itemids.json", m_itemIDs)))
		std::cerr << "Failed to read RS Item IDs. $ge command\
			will be disabled for this session." << std::endl;
}

GEReader::~GEReader() {};

bool GEReader::active() const
{
	return m_active;
}

Json::Value GEReader::getItem(std::string &name) const
{
	std::transform(name.begin(), name.end(), name.begin(), tolower);
	name[0] = toupper(name[0]);

	for (auto &val : m_itemIDs["items"]) {
		
		if (val["name"].asString() == name)
			return val;

		/* check item nicknames */
		if (val.isMember("nick")) {
			for (auto &nickname : val["nick"]) {
				if (nickname.asString() == name)
					return val;
			}
		}

	}

	/* return empty value if item not found */
	return Json::Value();
}
