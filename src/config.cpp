#include <algorithm>
#include <fstream>
#include <iostream>
#include "config.h"

#define MAX_SIZE 2048

static void removeLeading(std::string &s);
static bool blank(const std::string &s);

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
	std::string line, key, val;
	std::ifstream reader(m_path);
	int16_t nline, nsettings;
	size_t ind;
	bool block;

	nline = 0;
	nsettings = sizeof(settings) / sizeof(settings[0]);
	block = false;
	while (std::getline(reader, line)) {
		++nline;
		if (line[0] == '#' || line.empty())
			continue;
		if ((ind = line.find("=")) == std::string::npos) {
			std::cerr << m_path << ": line " << nline <<
				": invalid syntax" << std::endl;
			return false;
		}
		key = line.substr(0, ind);
		key.erase(std::remove_if(key.begin(), key.end(), isspace),
				key.end());
		for (ind = 0; ind < nsettings; ++ind) {
			if (key == settings[ind].key)
				break;
		}
		if (ind == nsettings) {
			std::cerr << m_path << ": line " << nline
				<< ": unrecognized key -- " << key << std::endl;
			return false;
		}
		if (settings[ind].val_type == STRING) {
			if ((val = parseString(line, ind)).empty()) {
				std::cerr << m_path << ": line " << nline << ": no "
					"setting provided for " << key << std::endl;
				return false;
			}
			m_settings[key] = val;
		}
		std::cout << key << std::endl << val << std::endl;
	}
	return true;
}

bool ConfigReader::write()
{
}

std::string ConfigReader::getSetting(const std::string &setting)
{
}

std::string ConfigReader::parseString(const std::string &buf, size_t ind)
{
	std::string val = buf.substr(buf.find("=") + 1);
	removeLeading(val);
	return val;
}

bool ConfigReader::parseList(const std::string &buf)
{
}

bool ConfigReader::parseOList(const std::string &buf)
{
}

/* removeLeading: remove all leading whitespace from string */
static void removeLeading(std::string &s)
{
	size_t ind = 0;
	while (ind < s.length() && (s[ind] == ' ' || s[ind] == '\t'))
		++ind;
	s = s.substr(ind);
}

static bool blank(const std::string &s)
{
}
