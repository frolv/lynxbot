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
				std::unordered_map<std::string, int> *names);
		~Moderator();
		bool active() const;
		bool validmsg(const std::string &msg, const char *nick,
				std::string &reason);
		uint8_t offenses(const std::string &nick) const;
		bool whitelist(const std::string &site);
		bool delurl(const std::string &site);
		void permit(char *nick, int amt);
		std::string fmt_whitelist() const;
		bool paste() const;
		bool log(int type, const char *user, const char *by,
				const char *reason);
		bool mod_action(int type, const char *name, const char *by,
				const char *reason, int len);
	private:
		const char *m_name;
		const char *m_channel;
		URLParser *m_parsep;
		ConfigReader *m_cfgr;
		struct client *m_client;
		std::unordered_map<std::string, int> *m_names;
		std::unordered_map<std::string, uint8_t> m_offenses;
		std::vector<std::string> m_whitelist;
		std::unordered_map<std::string, int> m_perm;
		bool m_active;
		bool m_ban_urls;
		uint32_t m_max_message_len;
		uint32_t m_max_pattern;
		uint32_t m_max_char;
		uint32_t m_cap_len;
		double m_cap_ratio;
		bool m_pastefmt;
		bool check_wl() const;
		bool check_spam(const std::string &msg) const;
		bool check_str(const std::string &msg, std::string &reason,
				char *logmsg) const;

};
