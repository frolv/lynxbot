#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <utils.h>
#include "lynxbot.h"
#include "Moderator.h"

Moderator::Moderator(URLParser *urlp, ConfigReader *cfgr)
	:m_parsep(urlp), m_cfgr(cfgr)
{
	std::string err;
	bool invalid;
	uint32_t cap_ratio;

	invalid = false;
	utils::split(m_cfgr->get("whitelist"), '\n', m_whitelist);
	m_pastefmt = m_cfgr->get("whitelist").length() > 300;
	if (!utils::parseBool(m_active, m_cfgr->get("enable_moderation"), err)) {
		std::cerr << m_cfgr->path() << ": enable_moderation: " << err
			<< " (defaulting to false)" << std::endl;
		m_active = false;
		invalid = true;
	}
	if (!utils::parseBool(m_ban_urls, m_cfgr->get("ban_urls"), err)) {
		std::cerr << m_cfgr->path() << ": ban_urls: " << err
			<< " (defaulting to true)" << std::endl;
		m_ban_urls = true;
		invalid = true;
	}
	if (!utils::parseInt(m_max_message_len, m_cfgr->get("max_message_len"),
				err)) {
		std::cerr << m_cfgr->path() << ": max_message_len: " << err
			<< " (defaulting to 300)" << std::endl;
		m_max_message_len = 300;
		invalid = true;
	}
	if (!utils::parseInt(m_max_pattern, m_cfgr->get("max_pattern"), err)) {
		std::cerr << m_cfgr->path() << ": max_pattern: " << err
			<< " (defaulting to 6)" << std::endl;
		m_max_pattern = 6;
		invalid = true;
	}
	if (!utils::parseInt(m_max_char, m_cfgr->get("max_char"), err)) {
		std::cerr << m_cfgr->path() << ": max_char: " << err
			<< " (defaulting to 15)" << std::endl;
		m_max_char = 15;
		invalid = true;
	}
	if (!utils::parseInt(m_cap_len, m_cfgr->get("cap_len"), err)) {
		std::cerr << m_cfgr->path() << ": cap_len: " << err
			<< " (defaulting to 30)" << std::endl;
		m_cap_len = 30;
		invalid = true;
	}
	if (!utils::parseInt(cap_ratio, m_cfgr->get("cap_ratio"), err)) {
		std::cerr << m_cfgr->path() << ": cap_ratio: " << err
			<< " (defaulting to 80)" << std::endl;
		m_cap_ratio = 0.8;
		invalid = true;
	} else {
		m_cap_ratio = (double)cap_ratio / 100.0;
	}
	if (invalid)
		WAIT_INPUT();
}

Moderator::~Moderator() {}

bool Moderator::active() const
{
	return m_active;
}

/* isValidMsg: check if msg is valid according to moderation settings */
bool Moderator::isValidMsg(const std::string &msg,
		const std::string &nick, std::string &reason)
{
	bool valid = true;

	if (msg.length() > m_max_message_len) {
		reason = "message too long!";
		valid = false;
	}
	if (valid && m_ban_urls && m_parsep->wasModified() && checkWhitelist()) {
		if (m_perm.find(nick) != m_perm.end() && m_perm[nick] != 0) {
			/* -1 indicates session long permission */
			if (m_perm[nick] != -1)
				m_perm[nick]--;
		} else {
			reason = "no posting links!";
			valid = false;
		}
	}
	if (valid && checkSpam(msg)) {
		reason = "no spamming words!";
		valid = false;
	}
	if (valid && checkString(msg, reason))
		valid = false;
	if (!valid) {
		/* update user's offenses if message is invalid */
		auto it = m_offenses.find(nick);
		if (it == m_offenses.end())
			m_offenses.insert({ nick, 1 });
		else
			it->second++;
	}
	/* if none of the above are found, the message is valid */
	return valid;
}

/* getOffenses: return how many offenses nick has committed */
uint8_t Moderator::getOffenses(const std::string &nick) const
{
	auto it = m_offenses.find(nick);
	return it == m_offenses.end() ? 0 : it->second;
}

/* whitelist: add site to whitelist */
bool Moderator::whitelist(const std::string &site)
{
	std::string wl;

	if (std::find(m_whitelist.begin(), m_whitelist.end(), site)
			!= m_whitelist.end())
		return false;
	m_whitelist.push_back(site);
	wl = m_cfgr->get("whitelist");
	wl += '\n' + site;
	m_pastefmt = wl.length() > 300;
	m_cfgr->set("whitelist", wl);
	m_cfgr->write();
	return true;
}

/* delurl: remove site from whitelist */
bool Moderator::delurl(const std::string &site)
{
	std::string wl;
	size_t i;

	wl = m_cfgr->get("whitelist");
	if ((i = wl.find(site)) == std::string::npos)
		return false;

	/* remove the newline too */
	wl.erase(i, site.length() + 1);
	m_pastefmt = wl.length() > 300;
	m_whitelist.erase(std::remove(m_whitelist.begin(), m_whitelist.end(),
				site), m_whitelist.end());
	m_cfgr->set("whitelist", wl);
	m_cfgr->write();
	return true;
}

/* permit: permit nick to post amt links */
void Moderator::permit(char *nick, int amt)
{
	char *s;

	for (s = nick; *s; ++s)
		*s = tolower(*s);
	m_perm[nick] = amt;
}

/* fmt_whitelist: return a formatted string of all whitelisted sites */
std::string Moderator::fmt_whitelist() const
{
	std::string output = "Whitelisted sites:";

	if (m_pastefmt)
		return output + "\n\n" + m_cfgr->get("whitelist");

	output += ' ';
	for (auto itr = m_whitelist.begin(); itr != m_whitelist.end(); ++itr) {
		/* add commas after all but last */
		output += *itr + (itr == m_whitelist.end() - 1 ? "." : ", ");
	}
	return output;
}

bool Moderator::paste() const
{
	return m_pastefmt;
}

/* checkWhitelist: check if last parsed URL is whitelisted */
bool Moderator::checkWhitelist() const
{
	std::string domain, sub;

	domain = m_parsep->getLast()->domain;
	sub = m_parsep->getLast()->subdomain + domain;

	return std::find(m_whitelist.begin(), m_whitelist.end(), domain)
		== m_whitelist.end() && std::find(m_whitelist.begin(),
		m_whitelist.end(), sub) == m_whitelist.end();
}

/* checkSpam: check if msg contains word spam */
bool Moderator::checkSpam(const std::string &msg) const
{
	std::regex spamRegex("(.{2,}\\b)\\1{"
			+ std::to_string(m_max_pattern - 1) + ",}");
	std::smatch match;
	return std::regex_search(msg.begin(), msg.end(), match, spamRegex);
}

/* checkString: check if msg contains excess caps or character spam */
bool Moderator::checkString(const std::string &msg, std::string &reason) const
{
	unsigned int caps, repeated, len;
	char last;

	caps = repeated = len = 0;
	last = '\0';
	for (auto &c : msg) {
		/* only count non-whitespace chars */
		if (!isspace(c))
			++len;
		if (c == last) {
			++repeated;
		} else {
			repeated = 0;
			last = c;
		}
		if (repeated == m_max_char) {
			reason = "no spamming characters!";
			return true;
		}
		caps += (c >= 'A' && c <= 'Z') ? 1 : 0;
	}
	/* messages longer than cap_len with over cap_ratio% caps are invalid */
	if (msg.length() > m_cap_len && (caps / (double)len > m_cap_ratio)) {
		reason = "turn off your caps lock!";
		return true;
	}
	return false;
}
