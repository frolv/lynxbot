#include "..\stdafx.h"

#define WHEEL_COOLDOWN 1200

SelectionWheel::SelectionWheel() {

	if (!utils::readJSON("wheel.json", m_data)) {
		std::cerr << "Could not read wheel.json. Wheel will be disabled for this session." << std::endl;
		m_active = false;
		std::cin.get();
	}
	else {
		std::string reqs[4] = { "name", "cmd", "desc", "usage" };
		for (auto &s : reqs) {
			if (!m_data.isMember("wheel" + s)) {
				std::cerr << "wheel" << s << " variable is missing from wheel.json. Wheel will be disabled for this session." << std::endl;
				m_active = false;
				std::cin.get();
				return;
			}
		}
		m_active = true;
	}

}

SelectionWheel::~SelectionWheel() {}

bool SelectionWheel::isActive() {
	return m_active;
}

bool SelectionWheel::valid(const std::string &category) const {
	return m_data["categories"].isMember(category);
}

std::string SelectionWheel::name() const {
	return m_data["wheelname"].asString();
}

std::string SelectionWheel::cmd() const {
	return m_data["wheelcmd"].asString();
}

std::string SelectionWheel::desc() const {
	return m_data["wheeldesc"].asString();
}

std::string SelectionWheel::usage() const {
	return m_data["wheelusage"].asString();
}

std::string SelectionWheel::choose(const std::string &nick, const std::string &category) {

	Json::Value &arr = m_data["categories"][category];
	if (!arr.isArray()) {
		std::cerr << "wheel.json is improperly configured. Wheel will be disabled.";
		m_active = false;
		return "";
	}

	// choose value at random from given category
	srand(static_cast<uint32_t>(time(nullptr)));
	uint16_t index = rand() % arr.size();
	std::string selection = arr[index].asString();
	add(nick, selection);

	return selection;

}

bool SelectionWheel::ready(const std::string &nick) const {
	return m_stored.find(nick) == m_stored.end() || time(nullptr) - lastUsed(nick) >= WHEEL_COOLDOWN;
}

void SelectionWheel::add(const std::string &nick, const std::string &selection) {

	if (m_stored.find(nick) != m_stored.end()) {
		// update if aleady exists
		auto &r = m_stored.find(nick)->second;
		r.first = selection;
		r.second = time(nullptr);
	}
	else {
		// create otherwise
		WheelMap::value_type val = { nick, std::make_pair(selection, time(nullptr)) };
		m_stored.insert(val);
	}

}

std::string SelectionWheel::selection(const std::string &nick) const {
	if (m_stored.find(nick) == m_stored.end()) {
		return "";
	}
	return m_stored.find(nick)->second.first;
}

std::time_t SelectionWheel::lastUsed(const std::string &nick) const {
	return m_stored.find(nick)->second.second;
}