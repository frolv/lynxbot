#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include "Giveaway.h"
#include "Utils.h"

Giveaway::Giveaway() :m_active(false)
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
			std::cerr << "Could not locate giveaway.txt. Giveaways will be disabled." << std::endl;
			std::cin.get();
		}
	}
}

Giveaway::~Giveaway() {}

bool Giveaway::readSettings()
{
	std::ifstream reader(utils::getApplicationDirectory() + "/giveaway/giveaway-settings.txt");
	if (!reader.is_open()) {
		std::cerr << "Could not locate giveaway-settings.txt" << std::endl;
		return false;
	}

	bool success = true;
	std::string line;
	while (std::getline(reader, line)) {
		// remove all whitespace
		line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
		// split the line into two parts
		std::string::size_type eq = line.find('=');
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
				if (num < 0) {
					throw std::invalid_argument("negative number");
				}
				m_followerLimit = num;
			}
			else if (category == "timer") {
				m_type[2] = parseBool(setting);
			}
			else if (category == "mins") {
				int num = std::stoi(setting);
				if (num < 0) {
					throw std::invalid_argument("negative number");
				}
				m_interval = num;
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
			std::cerr << category << ": invalid number - " << setting << std::endl;
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

bool Giveaway::parseBool(const std::string &s)
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