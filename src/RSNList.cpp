#include <fstream>
#include <iostream>
#include "RSNList.h"
#include "utils.h"

RSNList::RSNList()
{
	if (!readFile()) {
		std::cerr << "Could not read rsn.json" << std::endl;
		std::cin.get();
	}
}

bool RSNList::add(const std::string &nick, const std::string &rsn,
		std::string &err)
{
	Json::Value t, user, prev(Json::arrayValue);
	if (rsn == "lynx_titan") {
		err = "nice try.";
		return false;
	}
	if (rsn.length() > 12) {
		err = "invalid RSN - too long.";
		return false;
	}
	if (!(t = findByNick(nick)).empty()) {
		err = "you already have a RSN set.";
		return false;
	}
	if (!(t = findByRSN(rsn)).empty()) {
		err = "the RSN " + rsn + " is already taken by "
			+ t["nick"].asString() + ".";
		return false;
	}
	user["nick"] = nick;
	user["rsn"] = rsn;
	user["prev"] = prev;
	m_rsns["rsns"].append(user);
	writeFile();
	return true;
}

bool RSNList::edit(const std::string &nick, const std::string &rsn,
		std::string &err)
{
	Json::Value t;
	Json::Value &user = findByNick(nick);
	if (rsn.length() > 12) {
		err = "invalid RSN - too long.";
		return false;
	}
	if (!(t = findByRSN(rsn)).empty()) {
		err = "the RSN " + rsn + " is already taken by "
			+ t["nick"].asString() + ".";
		return false;
	}
	if (user.empty()) {
		err = "you don't have a RSN set.";
		return false;
	}
	user["rsn"] = rsn;
	writeFile();
	return true;
}

bool RSNList::del(const std::string &nick)
{
	Json::ArrayIndex ind = 0;
	Json::Value def, rem;
	while (ind < m_rsns["rsns"].size()) {
		Json::Value val = m_rsns["rsns"].get(ind, def);
		if (val["nick"] == nick) break;
		++ind;
	}

	if (ind == m_rsns["rsns"].size())
		return false;

	m_rsns["rsns"].removeIndex(ind, &rem);
	writeFile();
	return true;
}

std::string RSNList::getRSN(const std::string &nick)
{
	Json::Value user;
	if ((user = findByNick(nick)).empty())
		return "";
	return user["rsn"].asString();
}

std::string RSNList::getNick(const std::string &rsn)
{
	Json::Value user;
	if ((user = findByRSN(rsn)).empty())
		return "";
	return user["nick"].asString();
}

Json::Value &RSNList::findByNick(const std::string &nick)
{
	for (auto &val : m_rsns["rsns"]) {
		if (val["nick"].asString() == nick)
			return val;
	}
	return m_empty;
}

Json::Value &RSNList::findByRSN(const std::string &rsn)
{
	for (auto &val : m_rsns["rsns"]) {
		if (val["rsn"].asString() == rsn)
			return val;
	}
	return m_empty;
}

bool RSNList::readFile()
{
	if (!utils::readJSON("rsns.json", m_rsns) || !m_rsns.isMember("rsns")
			|| !m_rsns["rsns"].isArray())
		return false;
	for (auto &val : m_rsns["rsns"]) {
		if (!val.isMember("nick") && !val.isMember("curr")
			&& !val.isMember("prev")) {
			std::cerr << "rsns.json is improperly configured"
				<< std::endl;
			return false;
		}
	}
	return true;
}

void RSNList::writeFile()
{
	std::ofstream ccfile;
	ccfile.open(utils::configdir() + "/json/rsns.json");
	Json::StyledWriter sw;
	ccfile << sw.write(m_rsns);
	ccfile.close();
}
