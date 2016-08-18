#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <utils.h>
#include "lynxbot.h"
#include "Moderator.h"

#define LOG_LEN 256

Moderator::Moderator(const char *name, const char *channel, URLParser *urlp,
		ConfigReader *cfgr, struct client *cl,
		std::unordered_map<std::string, int> *names)
	: m_name(name), m_channel(channel), m_parsep(urlp), m_cfgr(cfgr),
	m_client(cl), m_names(names)
{
	std::string err;
	bool invalid;
	uint32_t cap_ratio;

	invalid = false;
	utils::split(m_cfgr->get("whitelist"), '\n', m_whitelist);
	m_pastefmt = m_cfgr->get("whitelist").length() > 300;
	if (!utils::parseBool(m_active, m_cfgr->get("enable_moderation"), err)) {
		fprintf(stderr, "%s: enable_moderation: %s (defaulting to "
				"false)\n", m_cfgr->path().c_str(), err.c_str());
		m_active = false;
		invalid = true;
	}
	if (!utils::parseBool(m_ban_urls, m_cfgr->get("ban_urls"), err)) {
		fprintf(stderr, "%s: ban_urls: %s (defaulting to true)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_ban_urls = true;
		invalid = true;
	}
	if (!utils::parseInt(m_max_message_len, m_cfgr->get("max_message_len"),
				err)) {
		fprintf(stderr, "%s: max_message_len: %s (defaulting to 300)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_max_message_len = 300;
		invalid = true;
	}
	if (!utils::parseInt(m_max_pattern, m_cfgr->get("max_pattern"), err)) {
		fprintf(stderr, "%s: max_pattern: %s (defaulting to 6)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_max_pattern = 6;
		invalid = true;
	}
	if (!utils::parseInt(m_max_char, m_cfgr->get("max_char"), err)) {
		fprintf(stderr, "%s: max_char: %s (defaulting to 15)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_max_char = 15;
		invalid = true;
	}
	if (!utils::parseInt(m_cap_len, m_cfgr->get("cap_len"), err)) {
		fprintf(stderr, "%s: cap_len: %s (defaulting to 30)\n",
				m_cfgr->path().c_str(), err.c_str());
		m_cap_len = 30;
		invalid = true;
	}
	if (!utils::parseInt(cap_ratio, m_cfgr->get("cap_ratio"), err)) {
		fprintf(stderr, "%s: cap_ratio: %s (defaulting to 80)\n",
				m_cfgr->path().c_str(), err.c_str());
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

/* validmsg: check if msg is valid according to moderation settings */
bool Moderator::validmsg(const std::string &msg, const char *nick,
		std::string &reason)
{
	char logmsg[LOG_LEN];
	bool valid = true;
	uint8_t off;

	if (msg.length() > m_max_message_len) {
		reason = "message too long!";
		_sprintf(logmsg, LOG_LEN, "message length of %lu characters "
				"exceeded limit of %u", msg.length(),
				m_max_message_len);
		valid = false;
	}
	if (valid && m_ban_urls && m_parsep->wasModified() && check_wl()) {
		if (m_perm.find(nick) != m_perm.end() && m_perm[nick] != 0) {
			/* -1 indicates session long permission */
			if (m_perm[nick] != -1)
				m_perm[nick]--;
		} else {
			reason = "no posting links!";
			_sprintf(logmsg, LOG_LEN, "posted unauthorized link: %s",
					m_parsep->getLast()->full.c_str());
			valid = false;
		}
	}
	if (valid && check_spam(msg)) {
		reason = "no spamming words!";
		_sprintf(logmsg, LOG_LEN, "pattern repeated in message "
				"more than limit of %u times", m_max_pattern);
		valid = false;
	}
	if (valid && check_str(msg, reason, logmsg))
		valid = false;
	if (!valid) {
		/* update user's offenses if message is invalid */
		auto it = m_offenses.find(nick);
		if (it == m_offenses.end()) {
			off = 1;
			m_offenses.insert({ nick, 1 });
		} else {
			off = ++(it->second);
		}
		log(off > 3 ? BAN : TIMEOUT, nick, m_name, logmsg);
	}
	/* if none of the above are found, the message is valid */
	return valid;
}

/* offenses: return how many offenses nick has committed */
uint8_t Moderator::offenses(const std::string &nick) const
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
bool Moderator::permit(char *nick, int amt)
{
	char *s;

	for (s = nick; *s; ++s)
		*s = tolower(*s);

	try {
		if (!m_names->at(nick))
			return false;
	} catch (std::out_of_range) {
		(*m_names)[nick] = 0;
		return false;
	}

	m_perm[nick] = amt;
	return true;
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

/* log: write a message detailing moderation action to log file */
bool Moderator::log(int type, const char *user, const char *by,
		const char *reason)
{
	FILE *f;
	char path[MAX_PATH];
	char buf[MAX_MSG];
	struct tm modtm;
	time_t t;

	_sprintf(path, MAX_PATH, "%s%s", utils::configdir().c_str(),
			utils::config("modlog").c_str());
	if (!(f = fopen(path, "a"))) {
		perror(path);
		return false;
	}
	t = time(nullptr);

#ifdef __linux__
	modtm = *localtime(&t);
#endif
#ifdef _WIN32
	localtime_s(&modtm, &t);
#endif
	strftime(buf, MAX_MSG, "%Y-%m-%d %R", &modtm);
	fprintf(f, "[%s] %s %s by %s. Reason: %s\n", buf, user,
			type == BAN ? "banned" : "timed out",
			by, reason ? reason : "none");

	fclose(f);
	return true;
}

/* mod_action: perform a moderation action on name */
bool Moderator::mod_action(int type, const char *name,
		const char *by, const char *reason, int len)
{
	char msg[MAX_MSG];

	try {
		if (!m_names->at(name))
			return false;
	} catch (std::out_of_range) {
		(*m_names)[name] = 0;
		return false;
	}

	if (type == TIMEOUT)
		_sprintf(msg, MAX_MSG, "/timeout %s %d", name, len);
	else
		_sprintf(msg, MAX_MSG, "/ban %s", name);

	send_msg(m_client, m_channel, msg);
	log(type, name, by, *reason ? reason : NULL);
	return true;
}

/* check_wl: check if last parsed URL is whitelisted */
bool Moderator::check_wl() const
{
	std::string domain, sub;

	domain = m_parsep->getLast()->domain;
	sub = m_parsep->getLast()->subdomain + domain;

	return std::find(m_whitelist.begin(), m_whitelist.end(), domain)
		== m_whitelist.end() && std::find(m_whitelist.begin(),
		m_whitelist.end(), sub) == m_whitelist.end();
}

/* check_spam: check if msg contains word spam */
bool Moderator::check_spam(const std::string &msg) const
{
	std::regex spamRegex("(.{2,}\\b)\\1{"
			+ std::to_string(m_max_pattern - 1) + ",}");
	std::smatch match;
	return std::regex_search(msg.begin(), msg.end(), match, spamRegex);
}

/* check_str: check if msg contains excess caps or character spam */
bool Moderator::check_str(const std::string &msg, std::string &reason,
		char *logmsg) const
{
	unsigned int caps, repeated, len;
	char last;
	double ratio;

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
			_sprintf(logmsg, LOG_LEN, "character '%c' repeated in "
					"message over limit of %u times",
					c, m_max_char);
			return true;
		}
		caps += (c >= 'A' && c <= 'Z') ? 1 : 0;
	}
	/* messages longer than cap_len with over cap_ratio% caps are invalid */
	ratio = caps / (double)len;
	if (msg.length() > m_cap_len && (ratio > m_cap_ratio)) {
		reason = "turn off your caps lock!";
		_sprintf(logmsg, LOG_LEN, "%u out of %u non-whitespace "
				"characters in message were uppercase",
				caps, len);
		return true;
	}
	return false;
}
