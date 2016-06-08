#include <cpr/cpr.h>
#include <json/json.h>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <utils.h>
#include "Giveaway.h"

Giveaway::Giveaway(const std::string &channel, time_t initTime,
		ConfigReader *cfgr)
	: m_cfgr(cfgr), m_active(false), m_channel(channel), m_currFollowers(0),
	m_lastRequest(initTime), m_interval(0)
{
	init(initTime, true);
	if (!readGiveaway()) {
		if (m_active) {
			std::cout << "nothing to give away!" << std::endl;
			std::cin.get();
		}
	}
}

Giveaway::~Giveaway() {}

bool Giveaway::init(time_t initTime, bool first)
{
	if (!readSettings())
		std::cin.get();
	if (!m_active) {
		if (!first)
			m_active = true;
		else
			return false;
	}
	/* followers */
	if (m_type[1]) {
		if (first)
			interactiveFollowers();
		else
			m_currFollowers = getFollowers();
	}
	/* timer */
	if (m_type[2])
		updateTimes(initTime);
	return true;
}

bool Giveaway::active() const
{
	return m_active;
}

bool Giveaway::activate(time_t initTime, std::string &reason)
{
	if (m_active) {
		reason = "giveaways are already active.";
		return false;
	}
	if (!init(initTime, false)) {
		reason = "failed to start giveaway. See console for details.";
		return false;
	}
	if (m_items.empty()) {
		reason = "nothing left to give away!";
		return false;
	}
	writeSettings();
	return true;
}

void Giveaway::deactivate()
{
	m_active = false;
	writeSettings();
}

void Giveaway::setFollowers(bool setting, uint32_t amt)
{
	m_type[1] = setting;
	if (amt)
		m_followerLimit = amt;
	writeSettings();
}

void Giveaway::setTimer(bool setting, time_t interval)
{
	m_type[2] = setting;
	if (interval)
		m_interval = interval;
	writeSettings();
}

/*
bool Giveaway::checkSubs()
{
	if (!m_active) {
		return false;
	}
	if (m_items.empty()) {
		m_active = false;
		return false;
	}
	return m_type[0];
}
*/

bool Giveaway::checkConditions(time_t curr)
{
	if (m_items.empty()) {
		m_active = false;
		return false;
	}
	/* check all conditions and update stored data */
	if (m_type[1] && curr >= m_lastRequest + 60) {
		/* followers */
		m_lastRequest = curr;
		uint32_t fol = getFollowers();
		std::cout << "Followers: " << fol << "(" << m_currFollowers
			+ m_followerLimit << ")\n" << std::endl;
		if (fol >= m_currFollowers + m_followerLimit) {
			m_currFollowers += m_followerLimit;
			m_reason = 1;
			return true;
		}
	}
	if (m_type[2]) {
		/* time based */
		if (curr > m_earliest) {
			time_t gap = m_latest - m_earliest;
			double prob =
				static_cast<double>(curr - m_earliest) / gap;
			std::random_device rd;
			std::mt19937 gen(rd());
			if (std::generate_canonical<double, 10>(gen) <= prob) {
				updateTimes(curr);
				m_reason = 2;
				return true;
			}
		}
	}
	return false;
}

std::string Giveaway::giveaway()
{
	std::string output = "[GIVEAWAY: ";
	output += m_reason == 1 ? "followers" : "timed";
	output += "] " + getItem() + " (next code in ";
	switch (m_reason) {
	case 0:
		/* subs */
		break;
	case 1:
		/* followers */
		output += std::to_string(m_followerLimit) + " followers";
		break;
	default:
		/* timed */
		output += "~" + std::to_string(m_interval / 60) + " minutes";
		break;
	}
	output += ")";
	return output;
}

uint32_t Giveaway::followers()
{
	return m_followerLimit;
}

time_t Giveaway::interval()
{
	return m_interval;
}

/* currentSettings: return a formatted string of current giveaway settings */
std::string Giveaway::currentSettings(int8_t type)
{
	std::string followers = "every " + std::to_string(m_followerLimit)
		+ " followers";
	std::string timed = "every " + std::to_string(m_interval / 60)
		+ " minutes";
	std::string output;

	switch (type) {
	case 0:
		break;
	case 1:
		output = "follower giveaways are currently ";
		if (!m_type[1])
			return output + "inactive.";
		output += "set to occur " + followers + ".";
		break;
	case 2:
		output = "timed giveaways are currently ";
		if (!m_type[2])
			return output + "inactive.";
		output += "set to occur " + timed + ".";
		break;
	default:
		output = "giveaways are currently ";
		if (!m_active)
			return output + "inactive.";
		output += " active and set to occur ";
		if (m_type[1])
			output += followers + (m_type[2] ? " and " : ".");
		if (m_type[2])
			output += timed + ".";
		if (!m_type[1] && !m_type[2])
			output += "never.";
		break;
	}

	return output;
}

/* getFollowers: read channel followers from Twitch API */
uint32_t Giveaway::getFollowers() const
{
	cpr::Response resp =
		cpr::Get(cpr::Url("https://api.twitch.tv/kraken/channels/"
				+ m_channel + "/follows?limit=1"),
		cpr::Header{{ "Client-ID", "kkjhmekkzbepq0pgn34g671y5nexap8" }});
	Json::Reader reader;
	Json::Value val;
	if (!reader.parse(resp.text, val)) {
		std::cerr << "Failed to get followers for #" + m_channel
			+ "." << std::endl;
		return 0;
	}
	return val["_total"].asInt();
}

/* interactiveFollowers: continuously prompt user to read followers */
void Giveaway::interactiveFollowers()
{
	while (!(m_currFollowers = getFollowers())) {
		char c;
		std::cout << "Try again? (y/n) ";
		while (std::cin >> c) {
			if (c == 'y' || c == 'Y') {
				break;
			} else if (c == 'n' || c == 'N') {
				m_type[1] = false;
				std::cout << "Follower giveaways will be "
					"disabled for this session.\n"
					<< std::endl;
				return;
			} else {
				std::cout << "Invalid option.\n"
					"Try again? (y/n) ";
			}
		}
	}
}

/* readSettings: read all giveaway settings */
bool Giveaway::readSettings()
{
	std::string err;
	bool valid;
	uint32_t interval;

	valid = true;
	if (!utils::parseBool(m_active, m_cfgr->get("giveaway_active"), err)) {
		std::cerr << m_cfgr->path() << ": giveaway_active: " << err
			<< " (defaulting to false)" << std::endl;
		m_active = false;
		valid = false;
	}
	if (!utils::parseBool(m_type[1], m_cfgr->get("follower_giveaway"), err)) {
		std::cerr << m_cfgr->path() << ": follower_giveaway: " << err
			<< " (defaulting to false)" << std::endl;
		m_type[1] = false;
		valid = false;
	}
	if (!utils::parseInt(m_followerLimit, m_cfgr->get("follower_limit"), err)) {
		std::cerr << m_cfgr->path() << ": follower_limit: " << err
			<< " (follower giveaways disabled)" << std::endl;
		m_type[1] = false;
		m_followerLimit = 10;
		valid = false;
	}
	if (!utils::parseBool(m_type[2], m_cfgr->get("timed_giveaway"), err)) {
		std::cerr << m_cfgr->path() << ": timed_giveaway: " << err
			<< " (defaulting to false)" << std::endl;
		m_type[2] = false;
		valid = false;
	}
	if (!utils::parseInt(interval, m_cfgr->get("time_interval"), err)) {
		std::cerr << m_cfgr->path() << ": time_interval: " << err
			<< " (timed giveaways disabled)" << std::endl;
		m_type[2] = false;
		interval = 15;
		valid = false;
	}
	m_interval = interval * 60;
	return valid;
}

/* readGiveaway: read giveaway items from file */
bool Giveaway::readGiveaway()
{
	std::string path = utils::configdir() + utils::config("giveaway");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		std::cerr << "could not read " << path << std::endl;
		return false;
	}
	std::string line;
	while (std::getline(reader, line))
		m_items.push_back(line);
	reader.close();
	return true;
}

/* writeGiveaway: write giveaway items to file */
void Giveaway::writeGiveaway() const
{
	std::string path = utils::configdir() + utils::config("giveaway");
	std::ofstream writer(path);
	if (writer.is_open()) {
		for (auto &s : m_items)
			writer << s << std::endl;
	}
	writer.close();
}

/* writeSettings: write giveaway settings to file */
void Giveaway::writeSettings() const
{
	m_cfgr->set("giveaway_active", m_active ? "true" : "false");
	m_cfgr->set("follower_giveaway", m_type[1] ? "true" : "false");
	m_cfgr->set("follower_limit", std::to_string(m_followerLimit));
	m_cfgr->set("timed_giveaway", m_type[2] ? "true" : "false");
	m_cfgr->set("time_interval", std::to_string(m_interval / 60));
	m_cfgr->write();
}

/* update times: update interval in which timed giveaway will occur */
void Giveaway::updateTimes(time_t curr)
{
	/* timed giveaways are done within an interval to allow for variation */
	m_earliest = curr + static_cast<time_t>(m_interval * 0.8);
	m_latest = curr + static_cast<time_t>(m_interval * 1.2);
}

/* getItem: return the last item in m_items */
std::string Giveaway::getItem()
{
	std::string item;
	if (!m_items.empty()) {
		item = m_items[m_items.size() - 1];
		m_items.pop_back();
		writeGiveaway();
	}
	return item;
}
