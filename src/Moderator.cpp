#include "stdafx.h"
#include "Moderator.h"
#include "URLParser.h"

Moderator::Moderator(URLParser *urlp) :m_parsep(urlp)
{
	std::ifstream whitelist(utils::getApplicationDirectory() + "\\whitelist.txt");
	if (whitelist.is_open()) {
		std::string line;
		while (std::getline(whitelist, line)) {
			m_whitelist.push_back(line);
		}
	}
	else {
		std::cerr << "File whitelist.txt not found. All chat URLs will be banned." << std::endl;
		std::cin.get();
	}
}

Moderator::~Moderator() {}

bool Moderator::isValidMsg(const std::string &msg, const std::string &nick, std::string &reason)
{
	bool valid = true;

	if (msg.length() > 300) {
		reason = "message too long!";
		valid = false;
	}
	if (valid && m_parsep->wasModified() && checkWhitelist()) {
		if (std::find(m_permitted.begin(), m_permitted.end(), nick) != m_permitted.end()) {
			// if user is permitted, allow the message and remove them from permitted list
			m_permitted.erase(std::remove(m_permitted.begin(), m_permitted.end(), nick), m_permitted.end());
		}
		reason = "no posting links!";
		valid = false;
	}
	if (valid && checkSpam(msg)) {
		reason = "no spamming words!";
		valid = false;
	}
	if (valid && checkString(msg, reason)) {
		valid = false;
	}
	if (!valid) {
		// update user's offenses if message is invalid
		auto it = m_offenses.find(nick);
		if (it == m_offenses.end()) {
			m_offenses.insert({ nick, 1 });
		}
		else {
			it->second++;
		}
	}
	// if none of the above are found, the message is valid
	return valid;
}

uint8_t Moderator::getOffenses(const std::string &nick) const
{
	auto it = m_offenses.find(nick);
	return it == m_offenses.end() ? 0 : it->second;
}

void Moderator::whitelist(const std::string &site)
{
	if (std::find(m_whitelist.begin(), m_whitelist.end(), site) == m_whitelist.end()) {
		m_whitelist.push_back(site);
	}
	std::ofstream writer(utils::getApplicationDirectory() + "\\whitelist.txt");
	for (auto &s : m_whitelist) {
		writer << s << std::endl;
	}
}

void Moderator::permit(const std::string &nick)
{
	if (std::find(m_permitted.begin(), m_permitted.end(), nick) == m_permitted.end()) {
		m_permitted.push_back(nick);
	}
}

std::string Moderator::getFormattedWhitelist() const
{
	std::string output = "Whitelisted sites: ";
	for (auto itr = m_whitelist.begin(); itr != m_whitelist.end(); ++itr) {
		// add commas after all but last
		output += *itr + (itr == m_whitelist.end() - 1 ? "." : ", ");
	}
	return output;
}

bool Moderator::checkWhitelist() const
{
	return std::find(m_whitelist.begin(), m_whitelist.end(), m_parsep->getLast()->domain) == m_whitelist.end();
}

bool Moderator::checkSpam(const std::string &msg) const
{
	std::regex spamRegex("(.{2,}\\b)\\1{5,}");
	std::smatch match;
	return std::regex_search(msg.begin(), msg.end(), match, spamRegex);
}

bool Moderator::checkString(const std::string &msg, std::string &reason) const
{
	uint16_t caps = 0, repeated = 0;
	char last = '\0';
	for (auto &c : msg) {
		if (c == last)
			++repeated;
		else {
			repeated = 0;
			last = c;
		}
		if (repeated > 15) {
			reason = "no spamming characters!";
			return true;
		}
		caps += (c >= 'A' && c <= 'Z') ? 1 : 0;
	}
	// messages longer than 12 characters with over 70% caps are invalid
	if (msg.length() > 30 && (caps / (double)msg.length() > 0.8)) {
		reason = "turn off your caps lock!";
		return true;
	}
	return false;
}