#include <unordered_map>
#include "skills.h"

/* map skill names to ids */
static const std::unordered_map<std::string, uint8_t> skillmap = {
	{ "overall", 0 },
	{ "attack", 1 },
	{ "defence", 2 },
	{ "strength", 3 },
	{ "hitpoints", 4 },
	{ "ranged", 5 },
	{ "prayer", 6 },
	{ "magic", 7 },
	{ "cooking", 8 },
	{ "woodcutting", 9 },
	{ "fletching", 10 },
	{ "fishing", 11 },
	{ "firemaking", 12 },
	{ "crafting", 13 },
	{ "smithing", 14 },
	{ "mining", 15 },
	{ "herblore", 16 },
	{ "agility", 17 },
	{ "thieving", 18 },
	{ "slayer", 19 },
	{ "farming", 20 },
	{ "runecrafting", 21 },
	{ "hunter", 22 },
	{ "construction", 23 }
};

/* map skill nicknames to ids */
static const std::unordered_map<std::string, uint8_t> nickmap = {
	{ "total", 0 },
	{ "att", 1 },
	{ "def", 2 },
	{ "str", 3 },
	{ "hp", 4 },
	{ "range", 5 },
	{ "pray", 6 },
	{ "mage", 7 },
	{ "cook", 8 },
	{ "wc", 9 },
	{ "fletch", 10 },
	{ "fish", 11 },
	{ "fm", 12 },
	{ "craft", 13 },
	{ "smith", 14 },
	{ "mining", 15 },
	{ "herb", 16 },
	{ "agil", 17 },
	{ "thiev", 18 },
	{ "slay", 19 },
	{ "farm", 20 },
	{ "rc", 21 },
	{ "hunt", 22 },
	{ "con", 23 }
};

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

/* skill_name: return name of skill id */
std::string skill_name(uint8_t id)
{
	std::string name;

	for (std::unordered_map<std::string, uint8_t>::const_iterator it =
			skillmap.begin(); it != skillmap.end(); ++it) {
		if (it->second == id) {
			name += toupper((char)(it->first[0]));
			name += it->first.substr(1);
			return name;
		}
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
