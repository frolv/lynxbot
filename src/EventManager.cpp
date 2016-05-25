#include <ctime>
#include <fstream>
#include <iostream>
#include <utils.h>
#include "EventManager.h"

EventManager::EventManager()
	: m_msg(false)
{
	readFile();
}

EventManager::~EventManager() {}

bool EventManager::messagesActive()
{
	return m_msg;
}

void EventManager::activateMessages()
{
	m_msg = true;
}

void EventManager::deactivateMessages()
{
	m_msg = false;
}

bool EventManager::addMessage(const std::string &message,
	time_t cooldown, bool write)
{
	if (m_messages.size() >= 5 || cooldown % 300 != 0)
		return false;
	m_messages.push_back({ message, cooldown });
	reloadMessages();
	if (write)
		writeFile();
	return true;
}

bool EventManager::delMessage(uint32_t id)
{
	if (id < 1 || id > m_messages.size())
		return false;
	auto it = m_messages.begin() + (id - 1);
	m_messages.erase(it);
	reloadMessages();
	writeFile();
	return true;
}

std::string EventManager::messageList() const
{
	std::string output = "(";
	output += m_msg ? "active" : "inactive";
	output += ") ";
	for (size_t i = 0; i < m_messages.size(); ++i) {
		/* only display first 35 characters of each message */
		const std::string &msg = m_messages[i].first;
		output += std::to_string(i + 1) + ": "
			+ (msg.length() < 35 ? msg : (msg.substr(0, 32) + "..."))
			+ " [" + std::to_string(m_messages[i].second / 60) + "m]"
			+ (i == m_messages.size() - 1 ? "" : ", ");
	}
	if (output.empty())
		output += "No recurring messages exist.";
	return output;
}

std::vector<std::pair<std::string, time_t>> *EventManager::messages()
{
	return &m_messages;
}

void EventManager::readFile()
{
	std::string path = utils::configdir() + utils::config("recurring");
	std::ifstream reader(path);
	if (reader.is_open()) {
		m_msg = true;
		std::string line;
		uint16_t lineNum = 0;
		while (std::getline(reader, line)) {
			if (++lineNum > 5) {
				std::cout << path << ": only reading "
					"first five messages." << std::endl;
				break;
			}
			/* format is "cd message" */
			std::string::size_type pos = line.find(' ');
			if (pos == std::string::npos || pos == line.length() - 1) {
				std::cerr << path << ": syntax error "
					"on line " << lineNum << std::endl;
				std::cin.get();
				continue;
			}
			time_t cooldown;
			try {
				cooldown = 60 * std::stoi(line.substr(0, pos));
				if (!addMessage(line.substr(pos + 1),
						cooldown, false)) {
					std::cerr << path << ": line " << lineNum
						<< ": cooldown must be multiple"
						" of 5 mins" << std::endl;
					std::cin.get();
				}
			} catch (std::invalid_argument) {
				std::cerr << path << ": invalid number on line "
					<< lineNum << ": "
					<< line.substr(0, pos) << std::endl;
				std::cin.get();
			}
		}
	}
}

void EventManager::writeFile()
{
	std::string path = utils::configdir() + utils::config("recurring");
	std::ofstream writer(path);
	if (writer.is_open()) {
		for (auto &p : m_messages)
			writer << p.second / 60 << ' ' << p.first << std::endl;
	}
}

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
