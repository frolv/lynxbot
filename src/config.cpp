#include "config.h"

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
	{ "submessage", STRING },
	{ "extra8ballresponses", LIST }
};

ConfigReader::ConfigReader(const std::string &path)
	: m_path(path)
{
}

bool ConfigReader::read()
{
}

bool ConfigReader::write()
{
}

std::string ConfigReader::getSetting(const std::string &setting)
{
}
