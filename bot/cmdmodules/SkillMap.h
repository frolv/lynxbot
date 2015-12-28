#pragma once

const std::map<std::string, uint8_t> skillMap = {
	{ "overall", 0 }, { "attack", 1 }, { "defence", 2 }, { "strength", 3 }, { "hitpoints", 4 }, { "range", 5 },
	{ "prayer", 6 }, { "magic", 7 }, { "cooking", 8 }, { "woodcutting", 9 }, { "fletching", 10 }, { "fishing", 11 }, 
	{ "firemaking", 12 }, { "crafting", 13 }, { "smithing", 14 }, { "mining", 15 }, { "herblore", 16 }, { "agility", 17 }, 
	{ "thieving", 18 }, { "slayer", 19 }, { "farming", 20 }, { "runecrafting", 21 }, { "hunter", 22 }, { "construction", 23 }
};

const std::map<std::string, uint8_t> skillNickMap = {
	{ "total", 0 }, { "att", 1 }, { "def", 2 }, { "str", 3 }, { "hp", 4 }, { "ranged", 5 },
	{ "pray", 6 }, { "mage", 7 }, { "cook", 8 }, { "wc", 9 }, { "fletch", 10 }, { "fish", 11 },
	{ "fm", 12 }, { "craft", 13 }, { "smith", 14 }, { "mining", 15 }, { "herb", 16 }, { "agil", 17 },
	{ "thiev", 18 }, { "slay", 19 }, { "farm", 20 }, { "rc", 21 }, { "hunt", 22 }, { "con", 23 }
};

std::string getSkillNick(uint8_t skillID);