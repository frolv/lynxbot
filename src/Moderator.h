#pragma once

#include <unordered_map>
#include "client.h"
#include "config.h"
#include "URLParser.h"

#define TIMEOUT	1
#define BAN	2

class ConfigReader;
class URLParser;

class Moderator {

	public:
		Moderator(const char *name, const char *channel,
				URLParser *urlp, ConfigReader *cfgr,
				struct client *cl,
				std::unordered_map<std::string, int> *users);
		~Moderator();
		bool active() const;
		bool validmsg(const std::string &msg, const char *nick,
				std::string &reason);
		uint8_t offenses(const std::string &nick) const;
		bool whitelist(const std::string &site);
		bool delurl(const std::string &site);
		bool permit(char *nick, int amt);
		std::string fmt_whitelist() const;
		bool paste() const;
		bool log(int type, const char *user, const char *by,
				const char *reason);
		bool mod_action(int type, const char *name, const char *by,
				const char *reason, int len);
	private:
		const char *bot_name;
		const char *bot_channel;
		URLParser *parser;
		ConfigReader *cfg;
		struct client *client;
		std::unordered_map<std::string, int> *active_users;
		std::unordered_map<std::string, uint8_t> offense_map;
		std::vector<std::string> whitelisted_sites;
		std::unordered_map<std::string, int> permitted;
		bool enabled;
		bool ban_urls;
		uint32_t max_message_len;
		uint32_t max_pattern;
		uint32_t max_char;
		uint32_t cap_len;
		double cap_ratio;
		bool pastefmt;
		bool check_wl() const;
		bool check_spam(const std::string &msg) const;
		bool check_str(const std::string &msg, std::string &reason,
				char *logmsg) const;

};
