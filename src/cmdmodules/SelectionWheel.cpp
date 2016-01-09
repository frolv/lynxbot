#include "..\stdafx.h"

#define WHEEL_COOLDOWN 7200

SelectionWheel::SelectionWheel() {

	Json::Reader reader;
	std::ifstream categoryStream(utils::getApplicationDirectory() + "\\wheelcategories.json", std::ifstream::binary);

	if (!reader.parse(categoryStream, m_data)) {
		std::cerr << reader.getFormattedErrorMessages() << std::endl;
		std::cerr << "Could not read wheelcategories.json. Wheel will be disabled for this session." << std::endl;
		m_active = false;
		std::cin.get();
	}
	else {
		m_active = true;
	}

}

SelectionWheel::~SelectionWheel() {}

bool SelectionWheel::valid(const std::string &category) {
	return !utils::startsWith(category, "wheel") && m_data.isMember(category);
}

std::string SelectionWheel::name() {
	return m_data["wheelname"].asString();
}

std::string SelectionWheel::cmd() {
	return m_data["wheelcmd"].asString();
}

std::string SelectionWheel::desc() {
	return m_data["wheeldesc"].asString();
}

std::string SelectionWheel::usage() {
	return m_data["wheelusage"].asString();
}

std::string SelectionWheel::choose(const std::string &nick, const std::string &category) {

	Json::Value &arr = m_data[category];
	if (!arr.isArray()) {
		std::cerr << "wheelcategories.json is improperly configured. Wheel will be disabled.";
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

bool SelectionWheel::ready(const std::string &nick) {
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

std::string SelectionWheel::selection(const std::string &nick) {
	if (m_stored.find(nick) == m_stored.end()) {
		return "";
	}
	return m_stored.find(nick)->second.first;
}

std::time_t SelectionWheel::lastUsed(const std::string &nick) {
	return m_stored.find(nick)->second.second;
}