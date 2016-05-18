#include "SkillMap.h"

std::string getSkillNick(uint8_t skillID)
{
	for (std::unordered_map<std::string, uint8_t>::const_iterator it =
			skillNickMap.begin(); it != skillNickMap.end(); ++it) {
		if (it->second == skillID)
			return it->first;
	}
	return "";
}
