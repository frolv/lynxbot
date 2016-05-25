#include <cpr/cpr.h>
#include <json/json.h>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <utils.h>
#include "Giveaway.h"

Giveaway::Giveaway(const std::string &channel, time_t initTime)
	: m_active(false), m_channel(channel), m_currFollowers(0),
	m_lastRequest(initTime), m_interval(0)
{
	if (!init(initTime, true))
		std::cin.get();
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
	if (!readSettings()) {
		std::cout << "giveaways will be disabled" << std::endl;
		return false;
	}
	if (!m_active) {
		if (!first) {
			m_active = true;
		} else {
			std::cout << "giveaways are currently inactive\n"
				<< std::endl;
			return false;
		}
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

uint16_t Giveaway::followers()
{
	return m_followerLimit;
}

time_t Giveaway::interval()
{
	return m_interval;
}

std::string Giveaway::currentSettings()
{
	std::string output = "giveaways are currently ";
	if (!m_active)
		return output + "inactive.";
	output += " active and set to occur ";
	if (m_type[1])
		output += "every " + std::to_string(m_followerLimit)
			+ " followers" + (m_type[2] ? " and " : ".");
	if (m_type[2])
		output += "every " + std::to_string(m_interval / 60)
			+ " minutes.";
	if (!m_type[1] && !m_type[2])
		output += "never.";
	return output;
}

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

void Giveaway::interactiveFollowers()
{
	/* followers */
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

bool Giveaway::readSettings()
{
	std::string path = utils::configdir() + utils::config("giveaway-settings");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		std::cerr << "could not locate " << path << std::endl;
		return false;
	}

	bool success = true;
	std::string line;
	uint8_t lineNum = 0;
	while (std::getline(reader, line)) {
		lineNum++;
		/* remove all whitespace */
		line.erase(std::remove_if(line.begin(), line.end(), isspace),
			line.end());
		/* split the line into two parts */
		std::string::size_type eq = line.find('=');
		if (eq == std::string::npos) {
			success = false;
			std::cerr << "Syntax error on line " << lineNum
				<< " of " << path << std::endl;
		}
		std::string category = line.substr(0, eq);
		std::string setting = line.substr(eq + 1);
		try {
			if (category == "active") {
				m_active = parseBool(setting);
			} /*else if (category == "subs") {
				m_type[0] = parseBool(setting);
			}*/ else if (category == "followers") {
				m_type[1] = parseBool(setting);
			} else if (category == "numfollowers") {
				int num = std::stoi(setting);
				if (num <= 0)
					throw std::invalid_argument(
							"negative number");
				m_followerLimit = num;
			} else if (category == "timer") {
				m_type[2] = parseBool(setting);
			} else if (category == "mins") {
				int num = std::stoi(setting);
				if (num <= 0)
					throw std::invalid_argument(
							"negative number");
				/* convert to seconds */
				m_interval = num * 60;
			} else {
				std::cerr << "Invalid category in " << path
					<< ": " << category << std::endl;
				success = false;
			}
		} catch (std::runtime_error &e) {
			std::cerr << category << ": " << e.what() << std::endl;
			success = false;
		} catch (std::invalid_argument) {
			std::cerr << category << ": invalid number - "
				<< setting << " (must be positive integer)"
				<< std::endl;
			success = false;
		}
	}
	reader.close();
	return success;
}

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

void Giveaway::writeSettings() const
{
	std::string path = utils::configdir() + utils::config("giveaway-settings");
	std::ofstream writer(path);
	if (writer.is_open()) {
		writer << "active = " << (m_active ? "true" : "false")
			<< std::endl;
		writer << "followers = " << (m_type[1] ? "true" : "false")
			<< std::endl;
		writer << "numfollowers = " << m_followerLimit << std::endl;
		writer << "timer = " << (m_type[2] ? "true" : "false")
			<< std::endl;
		writer << "mins = " << m_interval / 60 << std::endl;
	}
	writer.close();
}

bool Giveaway::parseBool(const std::string &s) const
{
	/* oh boy */
	if (s == "true") {
		return true;
	} else if (s == "false") {
		return false;
	} else {
		throw std::runtime_error("invalid setting - " + s
			+ " (must be true/false)");
	}
}

void Giveaway::updateTimes(time_t curr)
{
	/* timed giveaways are done within an interval to allow for variation */
	m_earliest = curr + static_cast<time_t>(m_interval * 0.8);
	m_latest = curr + static_cast<time_t>(m_interval * 1.2);
}

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
