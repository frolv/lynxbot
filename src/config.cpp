#include <algorithm>
#include <fstream>
#include <iostream>
#include <utils.h>
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
	std::string buf, line, key, val, err;
	std::ifstream reader(m_path);
	uint16_t nline, nsettings;
	size_t ind, set;
	int8_t open;

	nsettings = sizeof(settings) / sizeof(settings[0]);
	nline = open = set = 0;
	while (std::getline(reader, line)) {
		++nline;
		/* remove commented sections of lines */
		if ((ind = line.find('#')) != std::string::npos
				&& line[ind - 1] != '\\')
			line = line.substr(0, ind);
		if (blank(line))
			continue;
		if (!open) {
			removeLeading(line);
			if ((ind = line.find('=')) == std::string::npos) {
				err = "unrecognized token -- ";
				for (ind = 0; !isspace(line[ind]); ++ind)
					err += line[ind];
				std::cerr << m_path << ": line " << nline <<
					": " << err << std::endl;
				return false;
			}
			key = line.substr(0, ind);
			while (isspace(key.back()))
				key.pop_back();
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
				if (settings[set].val_type == LIST)
					val = parseList(val, err);
				else
					val = parseOList(key, val, err);
				if (val.empty()) {
					std::cerr << m_path << ": line "
						<< nline << ": " << err
						<< std::endl;
					return false;
				}
				m_settings[key] = val;
				buf = "";
			}
		}
	}
	if (open) {
		std::cerr << m_path << ": unexpected end of file" << std::endl;
		return false;
	}
	for (set = 0; set < nsettings; ++set) {
		if (m_settings.find(settings[set].key) == m_settings.end()) {
			std::cerr << m_path << ": missing required setting: "
				<< settings[set].key << std::endl;
			return false;
		}
	}
	return true;
}

bool ConfigReader::write()
{
}

std::string ConfigReader::get(const std::string &key)
{
	return m_settings[key];
}

void ConfigReader::set(const std::string &key, const std::string &val)
{
	m_settings[key] = val;
}

std::string ConfigReader::parseString(const std::string &buf)
{
	std::string val = buf.substr(buf.find('=') + 1);
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

std::string ConfigReader::parseOList(const std::string &key,
		const std::string &buf, std::string &err)
{
	std::string s, item;
	size_t end;

	s = buf.substr(1);
	while (s.back() != '}')
		s.pop_back();
	s.pop_back();
	while (isspace(s.back()))
		s.pop_back();

	end = 0;
	while (end != std::string::npos) {
		removeLeading(s);
		if ((end = s.find('}')) != std::string::npos) {
			item = s.substr(0, end + 1);
			if (!parseObj(key, item, err))
				return "";
			s = s.substr(end + 1);
			removeLeading(s);
			if (s[0] == ',') {
				s = s.substr(1);
				if (s.find('{') == std::string::npos) {
					err = "expected item after comma";
					return "";
				}
			} else if (s.find('{') != std::string::npos) {
				err = "unexpected list continuation";
				return "";
			}
		}
	}

	return "olist";
}

bool ConfigReader::parseObj(const std::string &key, std::string &obj,
		std::string &err)
{
	std::vector<std::string> items;
	std::string okey, oval;
	std::unordered_map<std::string, std::string> map;
	size_t ind;

	/* remove surrounding braces and whitespace */
	obj = obj.substr(1);
	removeLeading(obj);
	while (obj.back() != '}')
		obj.pop_back();
	obj.pop_back();
	while (isspace(obj.back()))
		obj.pop_back();

	utils::split(obj, '\n', items);

	for (std::string s : items) {
		if ((ind = s.find('=')) == std::string::npos) {
			err = "unrecognized token in list -- ";
			for (ind = 0; !isspace(s[ind]); ++ind)
				err += s[ind];
			return false;
		}
		okey = s.substr(0, ind);
		while (isspace(okey.back()))
			okey.pop_back();
		oval = parseString(s);
		map[okey] = oval;
	}
	m_olist[key].emplace_back(map);
	return true;
}

/* removeLeading: remove all leading whitespace from string */
static void removeLeading(std::string &s)
{
	size_t ind = 0;
	while (ind < s.length() && isspace(s[ind]))
		++ind;
	s = s.substr(ind);
}

/* blank: return true if line is empty or only contains whitespace */
static bool blank(const std::string &s)
{
	for (char c : s) {
		if (!isspace(c))
			return false;
	}
	return true;
}
