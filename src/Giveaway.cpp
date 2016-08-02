#include <cpr/cpr.h>
#include <json/json.h>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <utils.h>
#include "Giveaway.h"
#include "lynxbot.h"

#ifdef __linux__
# include <stdio.h>
# include <string.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

#define MAX_PATH 4096
#define MAX_LABL 256

static std::string mkimg(const std::string &item);

#ifdef __linux__
static int genimg(const char *conf, const char *code, const char *font);
static int mergeimg(const char *conf);
static std::string upload(const char *conf);

static const char *PTPB = "https://ptpb.pw";
#endif

Giveaway::Giveaway(const std::string &channel, time_t initTime,
		ConfigReader *cfgr)
	: m_cfgr(cfgr), m_active(false), m_images(false), m_channel(channel),
	m_currFollowers(0), m_lastRequest(initTime), m_interval(0)
{
	init(initTime, true);
	if (!readGiveaway()) {
		if (m_active) {
			std::cout << "nothing to give away!" << std::endl;
			WAIT_INPUT();
		}
	}
}

Giveaway::~Giveaway() {}

/* init: read giveaways settings and initialize giveaway variables */
bool Giveaway::init(time_t initTime, bool first)
{
	if (!readSettings())
		WAIT_INPUT();
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

/* activate: enable giveaways */
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

/* deactivate: disable giveaways */
void Giveaway::deactivate()
{
	m_active = false;
	writeSettings();
}

/* setFollowers: change giveaway follower settings */
void Giveaway::setFollowers(bool setting, uint32_t amt)
{
	m_type[1] = setting;
	if (amt)
		m_followerLimit = amt;
	writeSettings();
}

/* setTimer: change giveaway timer settings */
void Giveaway::setTimer(bool setting, time_t interval)
{
	m_type[2] = setting;
	if (interval) {
		m_interval = interval;
		updateTimes(time(NULL));
	}
	writeSettings();
}

/* setImages: turn images on or off */
void Giveaway::setImages(bool setting)
{
	m_images = setting;
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

/* checkConditions: check if giveaway conditions are satisfied */
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

/* giveaway: give away an item */
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
	case 3:
		output = "image-based giveaways are currently ";
		output += m_images ? "active." : "inactive.";
		break;
	default:
		if (m_active && m_images)
			output += "image-based ";
		output += "giveaways are currently ";
		if (!m_active)
			return output + "inactive.";
		output += "active and set to occur ";
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
	if (!utils::parseBool(m_images, m_cfgr->get("image_giveaways"), err)) {
		std::cerr << m_cfgr->path() << ": image_giveaways: " << err
			<< " (defaulting to false)" << std::endl;
		m_images = false;
		valid = false;
	}
#ifdef _WIN32
	if (m_images) {
		std::cout << "Image-based giveaways are not available on "
			"Windows systems" << std::endl;
		m_images = false;
	}
#endif
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
	while (std::getline(reader, line)) {
		if (line.length() > MAX_LABL - 7) {
			std::cerr << "giveaway item too long: " << line
				<< std::endl;
			continue;
		} else {
			m_items.push_back(line);
		}
	}
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
	m_cfgr->set("image_giveaways", m_images ? "true" : "false");
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
	if (m_images)
		item = mkimg(item);
	return item;
}

/* mkimg: transform item into an image, upload to ptpb and return url */
static std::string mkimg(const std::string &item)
{
#ifdef _WIN32
	std::cerr << "image-based giveaways are not available on Windows "
		"systems" << std::endl;
	return item;
#endif

#ifdef __linux__
	static const char *fonts[] = { "DejaVu-Sans-Bold", "DejaVu-Serif-Bold",
		"DejaVu-Serif-Bold-Italic" };
	std::string path, url;
	size_t i;

	path = utils::configdir();
	if (path.length() > MAX_PATH - 15) {
		fprintf(stderr, "%s: path too long\n", path.c_str());
		return item;
	}

	srand(time(NULL));
	i = rand() % (sizeof(fonts) / sizeof(fonts[0]));
	if (genimg(path.c_str(), item.c_str(), fonts[i])) {
		fprintf(stderr, "failed to generate code image\n");
		return item;
	}
	if (mergeimg(path.c_str())) {
		fprintf(stderr, "failed to impose image on background\n");
		return item;
	}
	url = upload(path.c_str());

	return url.empty() ? item : url;
#endif
}

#ifdef __linux__
/* genimg: generate an image containing the text of code */
static int genimg(const char *conf, const char *code, const char *font)
{
	char path[MAX_PATH];
	char label[MAX_LABL];
	int status;

	strcpy(path, conf);
	strcat(path, "/img/code.png");
	strcpy(label, "label:");
	strcat(label, code);

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;
	case 0:
		execl("/usr/bin/convert", "convert", "-font", font,
				"-pointsize", "40", "-gravity", "center",
				"-transparent", "white", label, path,
				(char *)NULL);
		perror("/usr/bin/convert");
		exit(1);
	default:
		wait(&status);
		return status >> 8;
	}
}

/* mergeimg: merge code image file with a background */
static int mergeimg(const char *conf)
{
	char code[MAX_PATH];
	char back[MAX_PATH];
	char *s;
	int status, i;

	strcpy(code, conf);
	strcat(code, "/img/code.png");

	s = strcpy(back, conf) + strlen(conf);
	i = rand() % 5;
	_sprintf(s, MAX_PATH, "/img/back%d.jpg", i);

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;
	case 0:
		execl("/usr/bin/composite", "composite", "-gravity", "center",
				code, back, code, (char *)NULL);
		perror("/usr/bin/composite");
		exit(1);
	default:
		wait(&status);
		return status >> 8;
	}
}

/* upload: upload code image file to ptpb */
static std::string upload(const char *conf)
{
	int pipefd[2], status, bytes;
	char path[MAX_PATH];
	char buf[MAX_PATH];
	char *s, *t;

	strcpy(path, "c=@");
	strcat(path, conf);
	strcat(path, "/img/code.png");

	if (pipe(pipefd)) {
		perror("pipe");
		return "";
	}

	switch (fork()) {
	case -1:
		perror("fork");
		return "";
	case 0:
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		execl("/usr/bin/curl", "curl", "-F", path, PTPB, (char *)NULL);
		perror("/usr/bin/curl");
		exit(1);
	default:
		close(pipefd[1]);
		if ((bytes = read(pipefd[0], buf, sizeof(buf) - 1)) < 0) {
			perror("read");
			return "";
		}
		buf[bytes] = '\0';
		wait(&status);
		/* extract the url of the paste */
		if (!(s = strstr(buf, "url:")))
			return "";
		s += 5;
		if (!(t = strchr(s, '\n')))
			return "";
		*t = '\0';
		return std::string(s);
	}
}
#endif
