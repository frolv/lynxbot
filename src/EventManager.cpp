#include <ctime>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "EventManager.h"

EventManager::EventManager(ConfigReader *cfgr)
	: m_init(time(nullptr)), m_cfgr(cfgr), m_msg(false)
{
	readFile();
	m_msg = m_messages.size() > 0;
}

EventManager::~EventManager() {}

/* messagesActive: return true if recurring messages are active */
bool EventManager::messagesActive()
{
	return m_msg;
}

/* activateMessages: enable recurring messages */
void EventManager::activateMessages()
{
	m_msg = true;
}

/* deactivateMessages: disable recurring messages */
void EventManager::deactivateMessages()
{
	m_msg = false;
}

/* addMessage: add a recurring message */
bool EventManager::addMessage(const std::string &message,
	time_t cooldown, bool write)
{
	if (m_messages.size() >= 5 || cooldown % 300 != 0 || cooldown > 3600)
		return false;
	m_messages.push_back({ message, cooldown });
	reloadMessages();
	if (write)
		writeFile();
	return true;
}

/* delMessage: delete a recurring message */
bool EventManager::delMessage(size_t id)
{
	if (id < 1 || id > m_messages.size())
		return false;
	auto it = m_messages.begin() + (id - 1);
	m_messages.erase(it);
	reloadMessages();
	writeFile();
	return true;
}

/* messageList: return a formatted string of all messages and intervals */
std::string EventManager::messageList() const
{
	std::string output = "(";
	output += m_msg ? "active" : "inactive";
	output += ") ";
	for (size_t i = 0; i < m_messages.size(); ++i) {
		/* only display first 35 characters of each message */
		output += std::to_string(i + 1) + ": "
			+ message(i, 35)
			+ (i == m_messages.size() - 1 ? "" : ", ");
	}
	if (output.empty())
		output += "No recurring messages exist.";
	return output;
}

/* message: return a single message and its interval */
std::string EventManager::message(size_t id, int lim) const
{
	std::string output;

	const std::string &msg = m_messages[id].first;
	if (lim == -1)
		output += msg;
	else
		output += msg.length() < (size_t)lim
			? msg : (msg.substr(0, lim - 3) + "...");
	output += " [" + std::to_string(m_messages[id].second / 60) + "m]";

	return output;
}

/* messages: return a pointer to messages vector */
std::vector<std::pair<std::string, time_t>> *EventManager::messages()
{
	return &m_messages;
}

time_t EventManager::init()
{
	return m_init;
}

/* readFile: read recurring messages from file */
void EventManager::readFile()
{
	size_t i, max;
	bool error, valid;
	uint32_t cd;
	std::string err;
	std::vector<std::unordered_map<std::string, std::string>> &recurring =
		m_cfgr->olist()["recurring"];

	max = recurring.size();
	if (max > 5) {
		max = 5;
		std::cout << m_cfgr->path() << ": only reading first five "
			<< "messages" << std::endl;
	}

	error = false;
	for (i = 0; i < max; ++i) {
		valid = true;
		auto map = recurring[i];

		if (map.find("message") == map.end()) {
			err = "no message provided";
			valid = false;
		}
		if (map.find("period") == map.end()) {
			err = "no period provided";
			valid = false;
		}

		if (valid) {
			if (!utils::parseInt(cd, map["period"], err)) {
				valid = false;
			} else if (!addMessage(map["message"], cd * 60, false)) {
				err = "cooldown must be multiple of 5 minutes "
					"and no longer than 60 minutes";
				valid = false;
			}
		}

		if (!valid) {
			error = true;
			std::cerr << m_cfgr->path() << ": recurring message "
				<< i + 1 << ": " << err << std::endl;
			std::cerr << "skipping message" << std::endl;
			continue;
		}
	}
	if (error)
		std::cin.get();
}

/* writeFile: write recurring messages to file */
void EventManager::writeFile()
{
	std::vector<std::unordered_map<std::string, std::string>> &recurring =
		m_cfgr->olist()["recurring"];
	recurring.clear();
	for (auto &p : m_messages) {
		std::unordered_map<std::string, std::string> map;
		map["message"] = p.first;
		map["period"] = std::to_string(p.second / 60);
		recurring.emplace_back(map);
	}
	m_cfgr->write();
}

/* reloadMessages: load all messages into timer */
void EventManager::reloadMessages()
{
	for (std::vector<std::string>::size_type i = 0;
			i < m_messages.size(); ++i) {
		/* update offsets for all existing messages and new message */
		std::string name = "msg" + std::to_string(i);
		remove(name);
		uint16_t offset =
			static_cast<uint16_t>(i * (300 / m_messages.size()));
		add(name, m_messages[i].second, time(nullptr) + offset);
	}
}
