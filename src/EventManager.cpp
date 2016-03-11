#include <ctime>
#include "EventManager.h"

EventManager::EventManager() {}

EventManager::~EventManager() {}

bool EventManager::addMessage(const std::string &message, time_t cooldown)
{
	if (m_messages.size() > 5 || cooldown % 300 != 0)
		return false;
	m_messages.push_back({ message, cooldown });
	for (std::vector<std::string>::size_type i = 0; i < m_messages.size(); ++i) {
		/* update offsets for all existing messages and new message */
		std::string name = "msg" + std::to_string(i);
		remove(name);
		uint16_t offset = static_cast<uint16_t>(i * (300 / m_messages.size()));
		add(name, m_messages[i].second, time(nullptr) + offset);
	}
	return true;
}

bool EventManager::delMessage(uint32_t id)
{
	if (id < 1 || id > m_messages.size())
		return false;
	auto it = m_messages.begin() + (id - 1);
	m_messages.erase(it);
	remove("msg" + std::to_string(id));
	return true;
}

std::string EventManager::messageList() const
{
	std::string output;
	for (size_t i = 0; i < m_messages.size(); ++i) {
		/* only display first 35 characters of each message */
		const std::string &msg = m_messages[i].first;
		output += std::to_string(i + 1) + ": " + (msg.length() < 35 ? msg : (msg.substr(0, 32) + "..."))
			+ (i == m_messages.size() - 1 ? "" : ", ");
	}
	if (output.empty()) {
		output += "No recurring messages exist.";
	}
	return output;
}

std::vector<std::pair<std::string, time_t>> *EventManager::messages()
{
	return &m_messages;
}