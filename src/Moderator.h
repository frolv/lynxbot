#pragma once

#include <unordered_map>
#include "config.h"
#include "URLParser.h"

#define TIMEOUT	0
#define BAN	1

class ConfigReader;
class URLParser;

class Moderator {

	public:
		Moderator(const char *name, URLParser *urlp, ConfigReader *cfgr,
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
		bool timeout(const char *name);
		bool ban(const char *name);
	private:
		const char *m_name;
		URLParser *m_parsep;
		ConfigReader *m_cfgr;
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
