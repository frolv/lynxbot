#include <ctype.h>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "lynxbot.h"
#include "RSNList.h"

RSNList::RSNList()
{
	if (!readFile()) {
		std::cerr << "Could not read rsns.json" << std::endl;
		WAIT_INPUT();
	}
}

/* add a new value with given nick and rsn */
bool RSNList::add(const std::string &nick, const std::string &rsn,
		std::string &err)
{
	Json::Value *t, user, prev(Json::arrayValue);
	if (!validRSN(rsn, err))
		return false;
	if (!(t = findByNick(nick))->empty()) {
		err = "you already have a RSN set";
		return false;
	}
	user["nick"] = nick;
	user["rsn"] = rsn;
	user["prev"] = prev;
	m_rsns["rsns"].append(user);
	utils::writeJSON("rsns.json", m_rsns);
	return true;
}

/* change the rsn associated with nick */
bool RSNList::edit(const std::string &nick, const std::string &rsn,
		std::string &err)
{
	Json::Value *user = findByNick(nick);
	if (!validRSN(rsn, err))
		return false;
	if (user->empty()) {
		err = "you don't have a RSN set";
		return false;
	}
	(*user)["prev"].append((*user)["rsn"].asString());
	(*user)["rsn"] = rsn;
	utils::writeJSON("rsns.json", m_rsns);
	return true;
}

/* delete saved data for nick */
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
	utils::writeJSON("rsns.json", m_rsns);
	return true;
}

/* get the rsn associated with a twitch nick */
const char *RSNList::getRSN(const char *nick)
{
	Json::Value *user;
	if ((user = findByNick(nick))->empty())
		return NULL;
	return (*user)["rsn"].asCString();
}

/* get the twitch nick associated with a rsn */
const char *RSNList::getNick(const char *rsn)
{
	Json::Value *user;
	if ((user = findByRSN(rsn))->empty())
		return NULL;
	return (*user)["nick"].asCString();
}

/* check if the given rsn is valid */
bool RSNList::validRSN(const std::string &rsn, std::string &err)
{
	Json::Value *t;
	for (char c : rsn) {
		if (!isalnum(c) && c != '_' && c != '-') {
			err = "'" + rsn + "' contains invalid characters";
			return false;
		}
	}
	if (rsn == "lynx_titan") {
		err = "nice try";
		return false;
	}
	if (rsn.length() > 12) {
		err = "invalid RSN - too long";
		return false;
	}
	if (!(t = findByRSN(rsn))->empty()) {
		err = "the RSN '" + rsn + "' is already taken by "
			+ (*t)["nick"].asString() + ".";
		return false;
	}
	return true;
}

/* find the value with the given nick if it exists */
Json::Value *RSNList::findByNick(const std::string &nick)
{
	for (auto &val : m_rsns["rsns"]) {
		if (val["nick"].asString() == nick)
			return &val;
	}
	return &m_empty;
}

/* find the value with the given rsn if it exists */
Json::Value *RSNList::findByRSN(const std::string &rsn)
{
	for (auto &val : m_rsns["rsns"]) {
		if (val["rsn"].asString() == rsn)
			return &val;
	}
	return &m_empty;
}

/* read the rsns file into m_rsns */
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
