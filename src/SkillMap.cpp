#include "stdafx.h"
#include "SkillMap.h"

std::string getSkillNick(uint8_t skillID)
{
	for (std::unordered_map<std::string, uint8_t>::const_iterator it = skillNickMap.begin(); it != skillNickMap.end(); ++it) {
		// this will be the full name
		if (it->second == skillID) {
			// increment the iterator again to get the nickname
			return it->first;
		}
	}
	return "";
}