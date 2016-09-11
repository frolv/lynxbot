#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <utils.h>
#include "config.h"
#include "lynxbot.h"

static void remove_leading(std::string &s);
static bool blank(const std::string &s);

static struct setting settings[] = {
	{ "name", STRING, "The Twitch username of the bot", true },
	{ "channel", STRING, "Channel you want to join", true },
	{ "password", STRING, "Oauth token for bot's Twitch account", true },
	{ "twitchtok", STRING, "Authorization token for your Twitch account\n"
		"If UNSET, LynxBot will go through an interactive authorization"
		" setup when run\nIf NULL, LynxBot will not try to access your "
		"Twitch account", true },
	{ "enable_moderation", STRING, "Whether LynxBot should moderate "
		"your chat", true },
	{ "ban_urls", STRING, "Whether to ban URLs in chat\nIf true, all non-"
		"whitelisted URLs will be banned", true },
	{ "max_message_len", STRING, "Maximum permitted length of a message "
		"(in characters)", true },
	{ "max_pattern", STRING, "Maximum amount of times a pattern can be "
		"consecutively repeated in a message", true },
	{ "max_char", STRING, "Maximum amount a single character can be "
		"consecutively repeated in a message", true },
	{ "cap_len", STRING, "Minimum message length at which capital letters "
		"start getting moderated", true },
	{ "cap_ratio", STRING, "Maximum percentage of capital letters allowed "
		"in message", true },
	{ "whitelist", LIST, "if ban_urls is set to true, any website not on "
		"this list will be banned", true },
	{ "giveaway_active", STRING, "Whether giveaways are enabled or "
		"disabled", true },
	{ "image_giveaways", STRING, "Whether to generate images containing "
		"giveaway items rather than posting\nthem in plaintext "
		"(GNU/Linux systems only)", true },
	{ "follower_giveaway", STRING, "Whether follower giveaways are "
		"enabled", true },
	{ "follower_limit", STRING, "Number of followers required for "
		"giveaway", true },
	{ "timed_giveaway", STRING, "Whether timed giveaways are enabled",
		true },
	{ "time_interval", STRING, "Interval in minutes at which timed "
		"giveaways occur", true },
	{ "auto_disable", STRING, "If true, recurring messages will "
		"automatically be disabled when\nthe stream goes "
		"offline (and enabled when it comes on)", true },
	{ "recurring", OLIST, "List of all recurring messages\nEach recurring "
		"message has a period in mins and a message\nThe period must "
		"be a multiple of 5 and less than 60\nA maximum of 5 "
		"recurring messages are allowed", true },
	{ "familiarity_mode", STRING, "If set to true, chat messages starting "
		"with a ! will also be processed\nas commands in addition to "
		"those starting with a $", true },
	{ "url_titles", STRING, "Whether to post webpage titles when URLs are "
		"posted in chat", true },
	{ "submessage", STRING, "Message posted in chat when someone subscribes"
		" or resubscribes, respectively\nThe following format sequences"
		" are accepted:\n\t%c - channel name\n\t%m - number of months "
		"person has subscribed in a row\n\t%n - Twitch nickname of the "
		"person who subscribed\n\t%N - equivalent to \"@%n,\" i.e. "
		"\"@NICK,\"\n\t%% - a literal '%'", false },
	{ "resubmessage", STRING, "", true },
	{ "extra8ballresponses", LIST, "Additional responses for the 8ball "
		"command", false }
};

ConfigReader::ConfigReader(const char *path)
	: config_path(path)
{
}

bool ConfigReader::read()
{
	std::string buf, line, key, val, err;
	std::ifstream reader(config_path);
	uint16_t nline, nsettings;
	size_t ind, set;
	int8_t open;

	nsettings = sizeof(settings) / sizeof(settings[0]);
	nline = set = open = 0;
	while (std::getline(reader, line)) {
		++nline;
		/* remove commented sections of lines */
		if ((ind = line.find('#')) != std::string::npos
				&& line[ind - 1] != '\\')
			line = line.substr(0, ind);
		if (blank(line))
			continue;
		if (!open) {
			remove_leading(line);
			if ((ind = line.find('=')) == std::string::npos) {
				err = "unrecognized token -- ";
				for (ind = 0; !isspace(line[ind]); ++ind)
					err += line[ind];
				std::cerr << config_path << ": line " << nline <<
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
				std::cerr << config_path << ": line " << nline
					<< ": unrecognized key -- " << key
					<< std::endl;
				return false;
			}
		}
		if (settings[set].val_type == STRING) {
			if ((val = parse_string(line)).empty()) {
				std::cerr << config_path << ": line " << nline <<
					": no setting provided for " << key
					<< std::endl;
				return false;
			}
			setmap[key] = val;
		} else {
			/* reading a list within a brace block */
			ind = -1;
			while ((ind = line.find('{', ind + 1)) != std::string::npos)
				++open;
			if (open) {
				remove_leading(line);
				buf += line + '\n';
				ind = -1;
				while ((ind = line.find('}', ind + 1))
						!= std::string::npos)
					--open;
			}
			if (!open) {
				if ((val = parse_string(buf)).empty()) {
					std::cerr << config_path << ": line "
						<< nline << ": no setting "
						"provided for " << key
						<< std::endl;
					return false;
				}
				if (settings[set].val_type == LIST)
					val = parse_list(val, err);
				else
					val = parse_olist(key, val, err);
				if (val.empty()) {
					std::cerr << config_path << ": line "
						<< nline << ": " << err
						<< std::endl;
					return false;
				}
				setmap[key] = val;
				buf = "";
			}
		}
	}
	if (open) {
		std::cerr << config_path << ": unexpected end of file" << std::endl;
		return false;
	}
	for (set = 0; set < nsettings; ++set) {
		if (setmap.find(settings[set].key) == setmap.end()) {
			std::cerr << config_path << ": missing required setting: "
				<< settings[set].key << std::endl;
			return false;
		}
	}
	return true;
}

/* write: write stored settings to file */
void ConfigReader::write()
{
	time_t t;
	std::tm tm;
	size_t set, nsettings;

	nsettings = sizeof(settings) / sizeof(settings[0]);
	t = time(nullptr);
	tm = *localtime(&t);
	std::ofstream writer(config_path);

	/* header */
	writer << "# LynxBot configfile" << std::endl;
	writer << "# File automatically generated by LynxBot at "
		<< std::put_time(&tm, "%R %a %d/%m/%Y") << std::endl;
	writer << "# Please see " << BOT_WEBSITE << "/manual/config.html"
		<< std::endl << "# for complete reference" << std::endl;

	/* general settings */
	writer << std::endl << "#### General Settings ####" << std::endl
		<< std::endl;
	for (set = 0; set < 4; ++set)
		write_setting(writer, set);

	/* moderation settings */
	writer << std::endl << "#### Moderation Settings ####" << std::endl
		<< std::endl;
	for (; set < 11; ++set)
		write_setting(writer, set);

	/* whitelist */
	writer << std::endl << "#### Whitelisted websites ####" << std::endl
		<< std::endl;
	write_setting(writer, set++);

	/* giveaway settings */
	writer << std::endl << "#### Giveaway settings ####" << std::endl
		<< std::endl;
	for (; set < 18; ++set)
		write_setting(writer, set);

	/* recurring */
	writer << std::endl << "#### Recurring messages ####" << std::endl
		<< std::endl;
	write_setting(writer, set++);
	write_setting(writer, set++);

	/* other */
	writer << std::endl << "#### Other settings ####" << std::endl
		<< std::endl;
	for (; set < nsettings; ++set)
		write_setting(writer, set);
}

std::string ConfigReader::get(const std::string &key)
{
	return setmap[key];
}

void ConfigReader::set(const std::string &key, const std::string &val)
{
	setmap[key] = val;
}

const char *ConfigReader::path()
{
	return config_path;
}

std::unordered_map<std::string, std::vector<std::unordered_map<
	std::string, std::string>>> &ConfigReader::olist()
{
	return olistmap;
}

std::string ConfigReader::parse_string(const std::string &buf)
{
	std::string val = buf.substr(buf.find('=') + 1);
	remove_leading(val);
	return val;
}

std::string ConfigReader::parse_list(const std::string &buf, std::string &err)
{
	std::string out, s, val;
	size_t ind, nl;

	/* remove surrounding braces */
	s = buf.substr(1);
	while (isspace(s.back()) || s.back() == '}')
		s.pop_back();

	ind = 0;
	while (ind != std::string::npos) {
		remove_leading(s);
		if ((ind = s.find(',')) == std::string::npos) {
			val = s;
		} else {
			val = s.substr(0, ind);
			if (ind == s.length() - 1) {
				err = "expected item after comma";
				return "";
			}
		}
		if ((nl = val.find('\n')) != std::string::npos) {
			err = "unexpected list continuation after ";
			err += val.substr(0, nl);
			return "";
		}
		out += val;
		if (ind != std::string::npos)
			out += '\n';
		s = s.substr(ind + 1);
	}
	return out;
}

std::string ConfigReader::parse_olist(const std::string &key,
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
		remove_leading(s);
		if ((end = s.find('}')) != std::string::npos) {
			item = s.substr(0, end + 1);
			if (!parse_obj(key, item, err))
				return "";
			s = s.substr(end + 1);
			remove_leading(s);
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

bool ConfigReader::parse_obj(const std::string &key, std::string &obj,
		std::string &err)
{
	std::vector<std::string> items;
	std::string okey, oval;
	std::unordered_map<std::string, std::string> map;
	size_t ind;

	/* remove surrounding braces and whitespace */
	obj = obj.substr(1);
	remove_leading(obj);
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
		oval = parse_string(s);
		map[okey] = oval;
	}
	olistmap[key].emplace_back(map);
	return true;
}

/* write_setting: write the setting specified by ind to writer */
void ConfigReader::write_setting(std::ofstream &writer, size_t ind)
{
	std::vector<std::string> list_elems;
	size_t i;

	if (!settings[ind].comment.empty()) {
		writer << "# ";
		for (char c : settings[ind].comment) {
			if (c == '\n')
				writer << std::endl << "# ";
			else
				writer << c;
		}
		writer << std::endl;
	}

	if (settings[ind].val_type == STRING) {
		writer << settings[ind].key << " = "
			<< setmap[settings[ind].key] << std::endl;
	} else if (settings[ind].val_type == LIST) {
		utils::split(setmap[settings[ind].key], '\n', list_elems);
		writer << settings[ind].key << " = {" << std::endl;
		for (i = 0; i < list_elems.size(); ++i)
			writer << '\t' << list_elems[i]
				<< (i == list_elems.size() - 1 ? "" : ",")
				<< std::endl;
		writer << '}' << std::endl;
	} else {
		writer << settings[ind].key << " = {" << std::endl;
		for (i = 0; i < olistmap[settings[ind].key].size(); ++i) {
			auto map = olistmap[settings[ind].key][i];
			writer << "\t{" << std::endl;
			for (auto p : map)
				writer << "\t\t" << p.first << " = " << p.second
					<< std::endl;
			writer << "\t}";
			if (i != olistmap[settings[ind].key].size() - 1)
				writer << ',';
			writer << std::endl;
		}
		writer << '}' << std::endl;
	}
	if (settings[ind].blank)
		writer << std::endl;
}

/* remove_leading: remove all leading whitespace from string */
static void remove_leading(std::string &s)
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
