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
	: bot_name(name), bot_channel(channel), parser(urlp), cfg(cfgr),
	client(cl), active_users(names)
{
	std::string err;
	bool invalid;
	uint32_t cratio;

	invalid = false;
	utils::split(cfg->get("whitelist"), '\n', whitelisted_sites);
	pastefmt = cfg->get("whitelist").length() > 300;
	if (!utils::parseBool(enabled, cfg->get("enable_moderation"), err)) {
		fprintf(stderr, "%s: enable_moderation: %s (defaulting to "
				"false)\n", cfg->path(), err.c_str());
		enabled = false;
		invalid = true;
	}
	if (!utils::parseBool(ban_urls, cfg->get("ban_urls"), err)) {
		fprintf(stderr, "%s: ban_urls: %s (defaulting to true)\n",
				cfg->path(), err.c_str());
		ban_urls = true;
		invalid = true;
	}
	if (!utils::parseInt(max_message_len, cfg->get("max_message_len"),
				err)) {
		fprintf(stderr, "%s: max_message_len: %s (defaulting to 300)\n",
				cfg->path(), err.c_str());
		max_message_len = 300;
		invalid = true;
	}
	if (!utils::parseInt(max_pattern, cfg->get("max_pattern"), err)) {
		fprintf(stderr, "%s: max_pattern: %s (defaulting to 6)\n",
				cfg->path(), err.c_str());
		max_pattern = 6;
		invalid = true;
	}
	if (!utils::parseInt(max_char, cfg->get("max_char"), err)) {
		fprintf(stderr, "%s: max_char: %s (defaulting to 15)\n",
				cfg->path(), err.c_str());
		max_char = 15;
		invalid = true;
	}
	if (!utils::parseInt(cap_len, cfg->get("cap_len"), err)) {
		fprintf(stderr, "%s: cap_len: %s (defaulting to 30)\n",
				cfg->path(), err.c_str());
		cap_len = 30;
		invalid = true;
	}
	if (!utils::parseInt(cratio, cfg->get("cratio"), err)) {
		fprintf(stderr, "%s: cratio: %s (defaulting to 80)\n",
				cfg->path(), err.c_str());
		cap_ratio = 0.8;
		invalid = true;
	} else {
		cap_ratio = (double)cratio / 100.0;
	}
	if (invalid)
		WAIT_INPUT();
}

Moderator::~Moderator() {}

bool Moderator::active() const
{
	return enabled;
}

/* validmsg: check if msg is valid according to moderation settings */
bool Moderator::validmsg(const std::string &msg, const char *nick,
		std::string &reason)
{
	char logmsg[LOG_LEN];
	bool valid = true;
	uint8_t off;

	if (msg.length() > max_message_len) {
		reason = "message too long!";
		snprintf(logmsg, LOG_LEN, "message length of %lu characters "
				"exceeded limit of %u", msg.length(),
				max_message_len);
		valid = false;
	}
	if (valid && ban_urls && parser->wasModified() && check_wl()) {
		if (permitted.find(nick) != permitted.end()
				&& permitted[nick] != 0) {
			/* -1 indicates session long permission */
			if (permitted[nick] != -1)
				permitted[nick]--;
		} else {
			reason = "no posting links!";
			snprintf(logmsg, LOG_LEN, "posted unauthorized link: %s",
					parser->getLast()->full.c_str());
			valid = false;
		}
	}
	if (valid && check_spam(msg)) {
		reason = "no spamming words!";
		snprintf(logmsg, LOG_LEN, "pattern repeated in message "
				"more than limit of %u times", max_pattern);
		valid = false;
	}
	if (valid && check_str(msg, reason, logmsg))
		valid = false;
	if (!valid) {
		/* update user's offenses if message is invalid */
		auto it = offense_map.find(nick);
		if (it == offense_map.end()) {
			off = 1;
			offense_map.insert({ nick, 1 });
		} else {
			off = ++(it->second);
		}
		log(off > 3 ? BAN : TIMEOUT, nick, bot_name, logmsg);
	}
	/* if none of the above are found, the message is valid */
	return valid;
}

/* offenses: return how many offenses nick has committed */
uint8_t Moderator::offenses(const std::string &nick) const
{
	auto it = offense_map.find(nick);
	return it == offense_map.end() ? 0 : it->second;
}

/* whitelist: add site to whitelist */
bool Moderator::whitelist(const std::string &site)
{
	std::string wl;

	if (std::find(whitelisted_sites.begin(), whitelisted_sites.end(), site)
			!= whitelisted_sites.end())
		return false;
	whitelisted_sites.push_back(site);
	wl = cfg->get("whitelist");
	wl += '\n' + site;
	pastefmt = wl.length() > 300;
	cfg->set("whitelist", wl);
	cfg->write();
	return true;
}

/* delurl: remove site from whitelist */
bool Moderator::delurl(const std::string &site)
{
	std::string wl;
	size_t i;

	wl = cfg->get("whitelist");
	if ((i = wl.find(site)) == std::string::npos)
		return false;

	/* remove the newline too */
	wl.erase(i, site.length() + 1);
	pastefmt = wl.length() > 300;
	whitelisted_sites.erase(std::remove(whitelisted_sites.begin(),
				whitelisted_sites.end(), site),
			whitelisted_sites.end());
	cfg->set("whitelist", wl);
	cfg->write();
	return true;
}

/* permit: permit nick to post amt links */
bool Moderator::permit(char *nick, int amt)
{
	char *s;

	for (s = nick; *s; ++s)
		*s = tolower(*s);

	try {
		if (!active_users->at(nick))
			return false;
	} catch (std::out_of_range) {
		(*active_users)[nick] = 0;
		return false;
	}

	permitted[nick] = amt;
	return true;
}

/* fmt_whitelist: return a formatted string of all whitelisted sites */
std::string Moderator::fmt_whitelist() const
{
	std::string output = "Whitelisted sites:";

	if (pastefmt)
		return output + "\n\n" + cfg->get("whitelist");

	output += ' ';
	for (auto itr = whitelisted_sites.begin(); itr != whitelisted_sites.end(); ++itr) {
		/* add commas after all but last */
		output += *itr + (itr == whitelisted_sites.end() - 1 ? "." : ", ");
	}
	return output;
}

bool Moderator::paste() const
{
	return pastefmt;
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

	snprintf(path, MAX_PATH, "%s%s", utils::configdir().c_str(),
			utils::config("modlog").c_str());
	if (!(f = fopen(path, "a"))) {
		perror(path);
		return false;
	}
	t = time(nullptr);

	modtm = *localtime(&t);
	strftime(buf, MAX_MSG, "%Y-%m-%d %H:%M", &modtm);
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
		if (!active_users->at(name))
			return false;
	} catch (std::out_of_range) {
		(*active_users)[name] = 0;
		return false;
	}

	if (type == TIMEOUT)
		snprintf(msg, MAX_MSG, "/timeout %s %d", name, len);
	else
		snprintf(msg, MAX_MSG, "/ban %s", name);

	send_msg(client, bot_channel, msg);
	log(type, name, by, *reason ? reason : NULL);
	return true;
}

/* check_wl: check if last parsed URL is whitelisted */
bool Moderator::check_wl() const
{
	std::string domain, sub;

	domain = parser->getLast()->domain;
	sub = parser->getLast()->subdomain + domain;

	return std::find(whitelisted_sites.begin(), whitelisted_sites.end(), domain)
		== whitelisted_sites.end() && std::find(whitelisted_sites.begin(),
		whitelisted_sites.end(), sub) == whitelisted_sites.end();
}

/* check_spam: check if msg contains word spam */
bool Moderator::check_spam(const std::string &msg) const
{
	std::regex spamRegex("(.{2,}\\b)\\1{"
			+ std::to_string(max_pattern - 1) + ",}");
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
		if (repeated == max_char) {
			reason = "no spamming characters!";
			snprintf(logmsg, LOG_LEN, "character '%c' repeated in "
					"message over limit of %u times",
					c, max_char);
			return true;
		}
		caps += (c >= 'A' && c <= 'Z') ? 1 : 0;
	}
	/* messages longer than cap_len with over cap_ratio% caps are invalid */
	ratio = caps / (double)len;
	if (msg.length() > cap_len && (ratio > cap_ratio)) {
		reason = "turn off your caps lock!";
		snprintf(logmsg, LOG_LEN, "%u out of %u non-whitespace "
				"characters in message were uppercase",
				caps, len);
		return true;
	}
	return false;
}
