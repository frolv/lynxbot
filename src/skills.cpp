#include "skills.h"

/* skill_nick: return nickname of skill id */
std::string skill_nick(uint8_t id)
{
	for (std::unordered_map<std::string, uint8_t>::const_iterator it =
			nickmap.begin(); it != nickmap.end(); ++it) {
		if (it->second == id)
			return it->first;
	}
	return "";
}

/* skill_id: return the id of skill or -1 if invalid */
int8_t skill_id(const std::string &skill)
{
	if (skillmap.find(skill) == skillmap.end()
			&& nickmap.find(skill) == nickmap.end())
		return -1;

	return skillmap.find(skill) == skillmap.end()
		? nickmap.find(skill)->second
		: skillmap.find(skill)->second;
}
