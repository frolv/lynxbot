#pragma once

#include <unordered_map>
#include "config.h"
#include "URLParser.h"

class ConfigReader;
class URLParser;

class Moderator {

	public:
		Moderator(URLParser *urlp, ConfigReader *cfgr);
		~Moderator();
		bool active() const;
		bool isValidMsg(const std::string &msg,
				const std::string &nick, std::string &reason);
		uint8_t getOffenses(const std::string &nick) const;
		bool whitelist(const std::string &site);
		void permit(const std::string &nick);
		std::string getFormattedWhitelist() const;
	private:
		URLParser *m_parsep;
		ConfigReader *m_cfgr;
		std::unordered_map<std::string, uint8_t> m_offenses;
		std::vector<std::string> m_whitelist;
		std::vector<std::string> m_permitted;
		bool m_active;
		bool m_ban_urls;
		uint32_t m_max_message_len;
		uint32_t m_max_pattern;
		uint32_t m_max_char;
		uint32_t m_cap_len;
		double m_cap_ratio;
		bool checkWhitelist() const;
		bool checkSpam(const std::string &msg) const;
		bool checkString(const std::string &msg,
				std::string &reason) const;

};
