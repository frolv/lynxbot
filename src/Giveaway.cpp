#include <cpr/cpr.h>
#include <json/json.h>
#include "stdafx.h"
#include "Giveaway.h"
#include "Utils.h"

Giveaway::Giveaway(const std::string &channel, time_t initTime)
	:m_active(false), m_channel(channel), m_lastRequest(initTime), m_currFollowers(0), m_interval(0)
{
	if (!readSettings()) {
		std::cout << "Giveaways will be disabled for this session." << std::endl;
		std::cin.get();
	}
	else if (!m_active) {
		std::cout << "Giveaways are currently inactive.\n" << std::endl;
	}
	else {
		if (!readGiveaway()) {
			m_active = false;
			std::cerr << "Could not locate giveaway.txt. Giveaways will be disabled." << std::endl;
			std::cin.get();
			return;
		}
		if (m_type[1]) {
			/* followers */
			while (!(m_currFollowers = getFollowers())) {
				char c;
				std::cout << "Try again? (y/n) ";
				while (std::cin >> c) {
					if (c == 'y' || c == 'Y') {
						break;
					}
					else if (c == 'n' || c == 'N') {
						m_type[1] = false;
						std::cout << "Follower giveaways will be disabled for this session.\n" << std::endl;
						return;
					}
					else {
						std::cout << "Invalid option.\nTry again? (y/n) " << std::endl;
					}
				}
			}
		}
		if (m_type[2]) {
			/* timer */
			updateTimes(initTime);
		}
	}
}

Giveaway::~Giveaway() {}

bool Giveaway::active() const
{
	return m_active;
}

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
		uint32_t fol;
		fol = getFollowers();
		std::cout << "Current: " << fol << "\nRequired: " << m_currFollowers + m_followerLimit << std::endl << std::endl;
		if ((fol/* = getFollowers()*/) >= m_currFollowers + m_followerLimit) {
			m_currFollowers += m_followerLimit;
			return true;
		}
	}
	if (m_type[2]) {
		/* time based */
		if (curr > m_earliest) {
			time_t gap = m_latest - m_earliest;
			double prob = static_cast<double>(curr - m_earliest) / gap;
			std::random_device rd;
			std::mt19937 gen(rd());
			if (std::generate_canonical<double, 10>(gen) <= prob) {
				updateTimes(curr);
				return true;
			}
		}
	}
	return false;
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

uint32_t Giveaway::getFollowers() const
{
	cpr::Response resp = cpr::Get(cpr::Url("https://api.twitch.tv/kraken/channels/" + m_channel + "/follows?limit=1"),
		cpr::Header{{ "Client-ID", "kkjhmekkzbepq0pgn34g671y5nexap8" }});
	Json::Reader reader;
	Json::Value val;
	if (!reader.parse(resp.text, val)) {
		std::cerr << "Failed to get followers for #" + m_channel + "." << std::endl;
		return 0;
	}
	return val["_total"].asInt();
}

bool Giveaway::readSettings()
{
	std::ifstream reader(utils::getApplicationDirectory() + "/giveaway/giveaway-settings.txt");
	if (!reader.is_open()) {
		std::cerr << "Could not locate giveaway-settings.txt" << std::endl;
		return false;
	}

	bool success = true;
	std::string line;
	uint8_t lineNum = 0;
	while (std::getline(reader, line)) {
		lineNum++;
		// remove all whitespace
		line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
		// split the line into two parts
		std::string::size_type eq = line.find('=');
		if (eq == std::string::npos) {
			success = false;
			std::cerr << "Syntax error on line " << lineNum << " of giveaway-settings.txt." << std::endl;
		}
		std::string category = line.substr(0, eq);
		std::string setting = line.substr(eq + 1);
		try {
			if (category == "active") {
				m_active = parseBool(setting);
			}
			else if (category == "subs") {
				m_type[0] = parseBool(setting);
			}
			else if (category == "followers") {
				m_type[1] = parseBool(setting);
			}
			else if (category == "numfollowers") {
				int num = std::stoi(setting);
				if (num <= 0) {
					throw std::invalid_argument("negative number");
				}
				m_followerLimit = num;
			}
			else if (category == "timer") {
				m_type[2] = parseBool(setting);
			}
			else if (category == "mins") {
				int num = std::stoi(setting);
				if (num <= 0) {
					throw std::invalid_argument("negative number");
				}
				// convert to seconds
				m_interval = num * 60;
			}
			else {
				std::cerr << "Invalid category in giveaway-settings.txt: " << category << std::endl;
				success = false;
			}
		}
		catch (std::runtime_error &e) {
			std::cerr << category << ": " << e.what() << std::endl;
			success = false;
		}
		catch (std::invalid_argument) {
			std::cerr << category << ": invalid number - " << setting << " (must be positive integer)" << std::endl;
			success = false;
		}
	}
	reader.close();
	return success;
}

bool Giveaway::readGiveaway()
{
	std::ifstream reader(utils::getApplicationDirectory() + "/giveaway/giveaway.txt");
	if (!reader.is_open()) {
		m_active = false;
		return false;
	}
	std::string line;
	while (std::getline(reader, line)) {
		m_items.push_back(line);
	}
	reader.close();
	return true;
}

void Giveaway::writeGiveaway() const
{
	std::ofstream writer(utils::getApplicationDirectory() + "/giveaway/giveaway.txt");
	if (writer.is_open()) {
		for (auto &s : m_items) {
			writer << s << std::endl;
		}
	}
	writer.close();
}

bool Giveaway::parseBool(const std::string &s) const
{
	// oh boy
	if (s == "true") {
		return true;
	}
	else if (s == "false") {
		return false;
	}
	else {
		throw std::runtime_error("invalid setting - " + s + " (must be true/false)");
	}
}

void Giveaway::updateTimes(time_t curr)
{
	m_earliest = curr + static_cast<time_t>(m_interval * 0.8);
	m_latest = curr + static_cast<time_t>(m_interval * 1.2);
}