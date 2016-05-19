#pragma once

#include <json/json.h>
#include <string>

class RSNList {

	public:
		RSNList();
		bool add(const std::string &nick, const std::string &rsn,
			std::string &err);
		bool edit(const std::string &nick, const std::string &rsn,
			std::string &err);
		bool del(const std::string &nick);
		std::string getRSN(const std::string &nick);
		std::string getNick(const std::string &rsn);
	private:
		Json::Value m_rsns;
		Json::Value m_empty;
		Json::Value &findByNick(const std::string &nick);
		Json::Value &findByRSN(const std::string &rsn);
		bool readFile();
		void writeFile();
};
