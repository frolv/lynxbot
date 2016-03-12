#pragma once

#include "URLParser.h"

class URLParser;

class Moderator {

	public:
		Moderator(URLParser *urlp);
		~Moderator();
		bool isValidMsg(const std::string &msg, const std::string &nick, std::string &reason);
		uint8_t getOffenses(const std::string &nick) const;
		void whitelist(const std::string &site);
		void permit(const std::string &nick);
		std::string getFormattedWhitelist() const;
	private:
		URLParser *m_parsep;
		std::unordered_map<std::string, uint8_t> m_offenses;
		std::vector<std::string> m_whitelist;
		std::vector<std::string> m_permitted;
		bool checkWhitelist() const;
		bool checkSpam(const std::string &msg) const;
		bool checkString(const std::string &msg, std::string &reason) const;

};