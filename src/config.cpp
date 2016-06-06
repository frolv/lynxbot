#include <algorithm>
#include <fstream>
#include <iostream>
#include "config.h"

#define MAX_SIZE 2048

static void removeLeading(std::string &s);
static bool comment(const std::string &s);
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
	std::string buf, line, key, val, err;
	std::ifstream reader(m_path);
	int16_t nline, nsettings;
	size_t ind, set;
	int8_t open;

	nsettings = sizeof(settings) / sizeof(settings[0]);
	nline = open = set = 0;
	while (std::getline(reader, line)) {
		++nline;
		if (comment(line) || line.empty())
			continue;
		if (!open) {
			if ((ind = line.find('=')) == std::string::npos) {
				std::cerr << m_path << ": line " << nline <<
					": invalid syntax" << std::endl;
				return false;
			}
			key = line.substr(0, ind);
			key.erase(std::remove_if(key.begin(), key.end(),
						isspace), key.end());
			for (set = 0; set < nsettings; ++set) {
				if (key == settings[set].key)
					break;
			}
			if (set == nsettings) {
				std::cerr << m_path << ": line " << nline
					<< ": unrecognized key -- " << key
					<< std::endl;
				return false;
			}
		}
		if (settings[set].val_type == STRING) {
			if ((val = parseString(line)).empty()) {
				std::cerr << m_path << ": line " << nline <<
					": no setting provided for " << key
					<< std::endl;
				return false;
			}
			m_settings[key] = val;
		} else {
			/* reading a list within a brace block */
			ind = -1;
			while ((ind = line.find('{', ind + 1)) != std::string::npos)
				++open;
			if (open) {
				removeLeading(line);
				buf += line + '\n';
				ind = -1;
				while ((ind = line.find('}', ind + 1))
						!= std::string::npos)
					--open;
			}
			if (!open) {
				if ((val = parseString(buf)).empty()) {
					std::cerr << m_path << ": line "
						<< nline << ": no setting "
						"provided for " << key
						<< std::endl;
					return false;
				}
				if ((val = parseList(val, err)).empty()) {
					std::cerr << m_path << ": line "
						<< nline << ": " << err
						<< std::endl;
					return false;
				}
				buf = "";
			}
		}
	}
	if (open) {
		std::cerr << m_path << ": unexpected end of file" << std::endl;
		return false;
	}
	return true;
}

bool ConfigReader::write()
{
}

std::string ConfigReader::getSetting(const std::string &setting)
{
}

std::string ConfigReader::parseString(const std::string &buf)
{
	std::string val = buf.substr(buf.find("=") + 1);
	removeLeading(val);
	return val;
}

std::string ConfigReader::parseList(const std::string &buf, std::string &err)
{
	std::string out, s, val;
	size_t ind, nl;

	/* remove surrounding braces */
	s = buf.substr(1);
	while (isspace(s.back()) || s.back() == '}')
		s.pop_back();

	ind = 0;
	while (ind != std::string::npos) {
		removeLeading(s);
		if ((ind = s.find(',')) == std::string::npos)
			val = s;
		else
			val = s.substr(0, ind);
		if ((nl = val.find('\n')) != std::string::npos) {
			err = "unrecognized token in list -- ";
			s = val.substr(nl + 1);
			nl = 0;
			while (nl < s.length() && !isspace(s[nl]))
				err += s[nl++];
			return "";
		}
		out += val + '\n';
		s = s.substr(ind + 1);
	}
	return out;
}

bool ConfigReader::parseOList(const std::string &buf)
{
}

/* removeLeading: remove all leading whitespace from string */
static void removeLeading(std::string &s)
{
	size_t ind = 0;
	while (ind < s.length() && isspace(s[ind]))
		++ind;
	s = s.substr(ind);
}

static bool comment(const std::string &s)
{
	size_t ind = 0;
	while (ind < s.length() && (s[ind] == ' ' || s[ind] == '\t'))
		++ind;
	return ind != s.length() && s[ind] == '#';
}

static bool blank(const std::string &s)
{
}
