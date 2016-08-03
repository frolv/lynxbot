#include <iostream>
#include <utils.h>
#include "lynxbot.h"
#include "SelectionWheel.h"

SelectionWheel::SelectionWheel()
{
	static const std::string reqs[6] = { "active", "name", "cmd",
		"desc", "usage", "cooldown"};

	srand(static_cast<uint32_t>(time(nullptr)));
	if (!utils::readJSON("wheel.json", m_data)) {
		std::cerr << "Could not read wheel.json. Wheel will be "
			"disabled for this session." << std::endl;
		m_active = false;
		WAIT_INPUT();
	} else {
		for (auto &s : reqs) {
			if (!m_data.isMember("wheel" + s)) {
				std::cerr << "wheel" << s << " variable is "
					"missing from wheel.json. Wheel will "
					"be disabled for this session."
					<< std::endl;
				m_active = false;
				WAIT_INPUT();
				return;
			}
		}
		m_active = m_data["wheelactive"].asBool();
		m_cooldown = m_data["wheelcooldown"].asInt() * 60;
	}
}

SelectionWheel::~SelectionWheel() {}

bool SelectionWheel::isActive()
{
	return m_active;
}

bool SelectionWheel::valid(const char *category) const
{
	return m_data["categories"].isMember(category);
}

const char *SelectionWheel::name() const
{
	return m_data["wheelname"].asCString();
}

const char *SelectionWheel::cmd() const
{
	return m_data["wheelcmd"].asCString();
}

const char *SelectionWheel::desc() const
{
	return m_data["wheeldesc"].asCString();
}

const char *SelectionWheel::usage() const
{
	return m_data["wheelusage"].asCString();
}

const char *SelectionWheel::choose(const char *nick, const char *category)
{
	int ind;
	const char *selection;

	Json::Value &arr = m_data["categories"][category];
	if (!arr.isArray()) {
		std::cerr << "wheel.json is improperly configured. "
			"Wheel will be disabled.";
		m_active = false;
		return "";
	}

	/* choose value at random from given category */
	ind = rand() % arr.size();
	selection = arr[ind].asCString();
	add(nick, selection);

	return selection;
}

bool SelectionWheel::ready(const char *nick) const
{
	return m_stored.find(nick) == m_stored.end()
		|| time(nullptr) - lastUsed(nick) >= m_cooldown;
}

void SelectionWheel::add(const std::string &nick, const std::string &selection)
{
	if (m_stored.find(nick) != m_stored.end()) {
		/* update if aleady exists */
		auto &r = m_stored.find(nick)->second;
		r.first = selection;
		r.second = time(nullptr);
	} else {
		/* create otherwise */
		WheelMap::value_type val =
			{ nick, std::make_pair(selection, time(nullptr)) };
		m_stored.insert(val);
	}
}

const char *SelectionWheel::selection(const char *nick) const
{
	if (m_stored.find(nick) == m_stored.end())
		return "";
	return m_stored.find(nick)->second.first.c_str();
}

time_t SelectionWheel::lastUsed(const std::string &nick) const
{
	return m_stored.find(nick)->second.second;
}
