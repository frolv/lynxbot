#include <fstream>
#include <iostream>
#include "config.h"

#define MAX_SIZE 2048

static struct setting settings[] = {
	{ "name", STRING },
	{ "channel", STRING },
	{ "password", STRING },
	{ "twitchtok", STRING },
	{ "enable_moderation", STRING },
	{ "ban_urls", STRING },
	{ "max_message_len", STRING },
	{ "max_pattern", STRING },
	{ "max_char", STRING },
	{ "cap_len", STRING },
	{ "cap_ratio", STRING },
	{ "whitelist", LIST },
	{ "giveaway_active", STRING },
	{ "follower_giveaway", STRING },
	{ "follower_limit", STRING },
	{ "timed_giveaway", STRING },
	{ "time_interval", STRING },
	{ "recurring", OLIST },
	{ "submessage", STRING },
	{ "extra8ballresponses", LIST }
};

ConfigReader::ConfigReader(const std::string &path)
	: m_path(path)
{
}

bool ConfigReader::read()
{
	std::string line;
	std::ifstream reader(m_path);
	while (std::getline(reader, line)) {
		if (line[0] == '#' || line.empty())
			continue;
		std::cout << line << std::endl;
	}
	return true;
}

bool ConfigReader::write()
{
}

std::string ConfigReader::getSetting(const std::string &setting)
{
}

bool ConfigReader::readString()
{
}

bool ConfigReader::readList()
{
}

bool ConfigReader::readOList()
{
}
