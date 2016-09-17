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

#define MAX_LABL 256

static std::string mkimg(const std::string &item);

const char *CHANNEL_API = "https://api.twitch.tv/kraken/channels/";

#ifdef __linux__
static int genimg(const char *conf, const char *code, const char *font);
static int mergeimg(const char *conf);
static std::string upload(const char *conf);

static const char *PTPB = "https://ptpb.pw";
#endif

Giveaway::Giveaway(const char *channel, time_t init_time, ConfigReader *cfgr)
	: cfg(cfgr), enabled(false), images(false), bot_channel(channel),
	curr_followers(0), last_check(init_time), timed_interval(0)
{
	init(init_time, true);
	if (!read_giveaway()) {
		if (enabled) {
			printf("Nothing to give away!\n");
			WAIT_INPUT();
		}
	}
}

Giveaway::~Giveaway() {}

/* init: read giveaways settings and initialize giveaway variables */
bool Giveaway::init(time_t init_time, bool first)
{
	if (!read_settings())
		WAIT_INPUT();
	if (!enabled) {
		if (!first)
			enabled = true;
		else
			return false;
	}
	/* followers */
	if (givtype[1]) {
		if (first)
			interactive_followers();
		else
			curr_followers = get_followers();
	}
	/* timer */
	if (givtype[2])
		update_times(init_time);
	return true;
}

bool Giveaway::active() const
{
	return enabled;
}

/* activate: enable giveaways */
bool Giveaway::activate(time_t init_time)
{
	if (enabled) {
		strcpy(error, "giveaways are already active.");
		return false;
	}
	if (!init(init_time, false)) {
		strcpy(error, "failed to start giveaway. See "
				"console for details.");
		return false;
	}
	if (items.empty()) {
		strcpy(error, "nothing left to give away!");
		return false;
	}
	write_settings();
	return true;
}

/* deactivate: disable giveaways */
void Giveaway::deactivate()
{
	enabled = false;
	write_settings();
}

/* set_followers: change giveaway follower settings */
void Giveaway::set_followers(bool setting, uint32_t amt)
{
	givtype[1] = setting;
	if (amt)
		follower_limit = amt;
	write_settings();
}

/* set_timer: change giveaway timer settings */
void Giveaway::set_timer(bool setting, time_t interval)
{
	givtype[2] = setting;
	if (interval) {
		timed_interval = interval;
		update_times(time(NULL));
	}
	write_settings();
}

/* set_images: turn images on or off */
void Giveaway::set_images(bool setting)
{
	images = setting;
	write_settings();
}

/*
bool Giveaway::check_subs()
{
	if (!enabled) {
		return false;
	}
	if (items.empty()) {
		enabled = false;
		return false;
	}
	return givtype[0];
}
*/

/* check: check if giveaway conditions are satisfied */
bool Giveaway::check(time_t curr)
{
	if (items.empty()) {
		enabled = false;
		return false;
	}
	/* check all conditions and update stored data */
	if (givtype[1] && curr >= last_check + 60) {
		/* followers */
		last_check = curr;
		uint32_t fol = get_followers();
		printf("Followers: %d (%d)\n", curr_followers, follower_limit);
		if (fol >= curr_followers + follower_limit) {
			curr_followers += follower_limit;
			reason = 1;
			return true;
		}
	}
	if (givtype[2]) {
		/* time based */
		if (curr > earliest) {
			time_t gap = latest - earliest;
			double prob =
				static_cast<double>(curr - earliest) / gap;
			std::random_device rd;
			std::mt19937 gen(rd());
			if (std::generate_canonical<double, 10>(gen) <= prob) {
				update_times(curr);
				reason = 2;
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
	output += reason == 1 ? "followers" : "timed";
	output += "] " + get_item() + " (next code in ";
	switch (reason) {
	case 0:
		/* subs */
		break;
	case 1:
		/* followers */
		output += std::to_string(follower_limit) + " followers";
		break;
	default:
		/* timed */
		output += "~" + std::to_string(timed_interval / 60) + " minutes";
		break;
	}
	output += ")";
	return output;
}

uint32_t Giveaway::followers()
{
	return follower_limit;
}

time_t Giveaway::interval()
{
	return timed_interval;
}

/* current_settings: return a formatted string of current giveaway settings */
std::string Giveaway::current_settings(int8_t type)
{
	std::string followers = "every " + std::to_string(follower_limit)
		+ " followers";
	std::string timed = "every " + std::to_string(timed_interval / 60)
		+ " minutes";
	std::string output;

	switch (type) {
	case 0:
		break;
	case 1:
		output = "follower giveaways are currently ";
		if (!givtype[1])
			return output + "inactive.";
		output += "set to occur " + followers + ".";
		break;
	case 2:
		output = "timed giveaways are currently ";
		if (!givtype[2])
			return output + "inactive.";
		output += "set to occur " + timed + ".";
		break;
	case 3:
		output = "image-based giveaways are currently ";
		output += images ? "active." : "inactive.";
		break;
	default:
		if (enabled && images)
			output += "image-based ";
		output += "giveaways are currently ";
		if (!enabled)
			return output + "inactive.";
		output += "active and set to occur ";
		if (givtype[1])
			output += followers + (givtype[2] ? " and " : ".");
		if (givtype[2])
			output += timed + ".";
		if (!givtype[1] && !givtype[2])
			output += "never.";
		break;
	}

	return output;
}

char *Giveaway::err()
{
	return error;
}

/* get_followers: read channel followers from Twitch API */
uint32_t Giveaway::get_followers() const
{
	cpr::Response resp;
	Json::Reader reader;
	Json::Value val;
	char url[256];

	snprintf(url, 256, "%s%s/follows?limit=1", CHANNEL_API, bot_channel);
	resp = cpr::Get(cpr::Url(url), cpr::Header{
			{ "Client-ID", "kkjhmekkzbepq0pgn34g671y5nexap8" }});
	if (!reader.parse(resp.text, val)) {
		fprintf(stderr, "Failed to get followers for #%s.",
				bot_channel);
		return 0;
	}
	return val["_total"].asInt();
}

/* interactive_followers: continuously prompt user to read followers */
void Giveaway::interactive_followers()
{
	int c;

	while (!(curr_followers = get_followers())) {
		printf("Try again? (y/n) ");
		while ((c = getchar()) != EOF) {
			if (c == 'y' || c == 'Y') {
				break;
			} else if (c == 'n' || c == 'N') {
				givtype[1] = false;
				printf("Follower giveaways will be disabled "
						"for this session\n\n");
				return;
			} else {
				printf("Invalid option.\nTry again? (y/n) ");
			}
		}
		if (c == EOF)
			exit(0);
	}
}

/* read_settings: read all giveaway settings */
bool Giveaway::read_settings()
{
	std::string err;
	bool valid;
	uint32_t interval;

	valid = true;
	if (!utils::parseBool(enabled, cfg->get("giveaway_active"), err)) {
		fprintf(stderr, "%s: giveaway_active: %s (defaulting to "
				"false)\n", cfg->path(), err.c_str());
		enabled = false;
		valid = false;
	}
	if (!utils::parseBool(images, cfg->get("image_giveaways"), err)) {
		fprintf(stderr, "%s: image_giveaways: %s (defaulting to "
				"false)\n", cfg->path(), err.c_str());
		images = false;
		valid = false;
	}
#ifdef _WIN32
	if (images) {
		printf("Image-based giveaways are not available "
				"on Windows systems\n");
		images = false;
	}
#endif
	if (!utils::parseBool(givtype[1], cfg->get("follower_giveaway"), err)) {
		fprintf(stderr, "%s: follower_giveaway: %s (defaulting to "
				"false)\n", cfg->path(), err.c_str());
		givtype[1] = false;
		valid = false;
	}
	if (!utils::parseInt(follower_limit, cfg->get("follower_limit"), err)) {
		fprintf(stderr, "%s: follower_limit: %s (follower giveaways "
				"disabled\n", cfg->path(), err.c_str());
		givtype[1] = false;
		follower_limit = 10;
		valid = false;
	}
	if (!utils::parseBool(givtype[2], cfg->get("timed_giveaway"), err)) {
		fprintf(stderr, "%s: timed_giveaway: %s (defaulting to false)\n",
				cfg->path(), err.c_str());
		givtype[2] = false;
		valid = false;
	}
	if (!utils::parseInt(interval, cfg->get("time_interval"), err)) {
		fprintf(stderr, "%s: time_interval: %s (timed giveaways "
				"disabled)\n", cfg->path(), err.c_str());
		givtype[2] = false;
		interval = 15;
		valid = false;
	}
	timed_interval = interval * 60;
	return valid;
}

/* read_giveaway: read giveaway items from file */
bool Giveaway::read_giveaway()
{
	std::string path = utils::configdir() + utils::config("giveaway");
	std::ifstream reader(path);
	if (!reader.is_open()) {
		fprintf(stderr, "Could not read %s\n", path.c_str());
		return false;
	}
	std::string line;
	while (std::getline(reader, line)) {
		if (line.length() > MAX_LABL - 7) {
			fprintf(stderr, "%s: giveaway item too long: %s\n",
					path.c_str(), line.c_str());
			continue;
		} else {
			items.push_back(line);
		}
	}
	reader.close();
	return true;
}

/* write_giveaway: write giveaway items to file */
void Giveaway::write_giveaway() const
{
	std::string path = utils::configdir() + utils::config("giveaway");
	std::ofstream writer(path);
	if (writer.is_open()) {
		for (auto &s : items)
			writer << s << std::endl;
	}
	writer.close();
}

/* write_settings: write giveaway settings to file */
void Giveaway::write_settings() const
{
	cfg->set("giveaway_active", enabled ? "true" : "false");
	cfg->set("image_giveaways", images ? "true" : "false");
	cfg->set("follower_giveaway", givtype[1] ? "true" : "false");
	cfg->set("follower_limit", std::to_string(follower_limit));
	cfg->set("timed_giveaway", givtype[2] ? "true" : "false");
	cfg->set("time_interval", std::to_string(timed_interval / 60));
	cfg->write();
}

/* update times: update interval in which timed giveaway will occur */
void Giveaway::update_times(time_t curr)
{
	/* timed giveaways are done within an interval to allow for variation */
	earliest = curr + static_cast<time_t>(timed_interval * 0.8);
	latest = curr + static_cast<time_t>(timed_interval * 1.2);
}

/* get_item: return the last item in items */
std::string Giveaway::get_item()
{
	std::string item;
	if (!items.empty()) {
		item = items[items.size() - 1];
		items.pop_back();
		write_giveaway();
	}
	if (images)
		item = mkimg(item);
	return item;
}

/* mkimg: transform item into an image, upload to ptpb and return url */
static std::string mkimg(const std::string &item)
{
#ifdef _WIN32
	fprintf(stderr, "image-based giveaways are not available "
			"on Windows systems\n");
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
	snprintf(s, MAX_PATH, "/img/back%d.jpg", i);

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
#endif /* __linux__ */
