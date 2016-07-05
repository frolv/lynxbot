#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#ifdef __linux__
# include <stdlib.h>
# include <unistd.h>
#endif

#ifdef _WIN32
# include <Windows.h>
#endif

#include <utils.h>

#define MAX_PATH_LEN 8192

static std::unordered_map<std::string, std::string> configs = {
	{ "giveaway", "/giveaway" },
	{ "twitter", "/twitter" },
	{ "config", "/config" },
	{ "submit", "/submitted" },
};

bool utils::startsWith(const std::string &str, const std::string &prefix)
{
	return str.find(prefix) == 0;
}

bool utils::endsWith(const std::string &str, const std::string &suffix)
{
	return str.find(suffix) == str.length() - suffix.length();
}

std::vector<std::string> &utils::split(const std::string &str,
		char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (!item.empty())
			elems.push_back(item);
	}
	return elems;
}

std::string utils::formatInteger(std::string integer)
{
	int pos = integer.length() - 3;
	if (pos < 1)
		return integer;
	while (pos > 0) {
		integer.insert(pos, ",");
		pos -= 3;
	}
	return integer;
}

#ifdef _WIN32
static std::string configdir_win()
{
	char buf[MAX_PATH_LEN];
	int bytes;

	if (!(bytes = GetModuleFileName(NULL, buf, sizeof(buf)))) {
		std::cerr << "could not get current directory" << std::endl;
		return "";
	}

	/* strip filename from path */
	std::string path(buf);
	path = path.substr(0, path.find_last_of("\\"));

	return path;
}
#endif

#ifdef __linux__
static std::string configdir_unix()
{
	char *h;
	if (!(h= getenv("HOME"))) {
		std::cerr << "could not read $HOME" << std::endl;
		return "";
	}
	return std::string(h) + "/.lynxbot";
}
#endif

std::string utils::configdir()
{
#ifdef __linux__
	return configdir_unix();
#endif
#ifdef _WIN32
	return configdir_win();
#endif
}

std::string utils::config(const std::string &cfg)
{
	std::string path = "";
	if (configs.find(cfg) != configs.end()) {
		path += configs[cfg];
#ifdef _WIN32
		std::replace(path.begin(), path.end(), '/', '\\');
		if (cfg != "twitter")
			path += ".txt";
#endif
	}
	return path;
}

bool utils::readJSON(const std::string &filename, Json::Value &val)
{
	Json::Reader reader;
	std::ifstream fileStream(configdir() + "/json/" + filename,
		std::ifstream::binary);

	if (!reader.parse(fileStream, val)) {
		std::cerr << reader.getFormattedErrorMessages() << std::endl;
		return false;
	}
	return true;
}

void utils::writeJSON(const std::string &filename, Json::Value &val)
{
	std::ofstream ofile(configdir() + "/json/" + filename);
	Json::StyledWriter sw;
	ofile << sw.write(val);
	ofile.close();
}

bool utils::parseBool(bool &b, const std::string &s, std::string &err)
{
	if (s == "true") {
		b = true;
		return true;
	}
	if (s == "false") {
		b = false;
		return true;
	}
	err = "invalid setting -- " + s;
	return false;
}

bool utils::parseInt(uint32_t &i, const std::string &s, std::string &err)
{
	try {
		i = std::stoi(s);
		return true;
	} catch (std::invalid_argument) {
		err = "invalid number -- " + s;
		return false;
	}
}

#define MIN	 60
#define HOUR	 3600
#define DAY	 86400

/* conv_time: convert t to days, hours, minutes and seconds */
std::string utils::conv_time(time_t t)
{
	time_t d, h, m;
	std::string out;

	d = t / DAY;
	t %= DAY;
	h = t / HOUR;
	t %= HOUR;
	m = t / MIN;
	t %= MIN;

	if (d)
		out += std::to_string(d) + " day" + (d == 1 ? "" : "s") + ", ";
	if (h)
		out += std::to_string(h) + " hour" + (h == 1 ? "" : "s") + ", ";
	if (m)
		out += std::to_string(m) + " minute" + (m == 1 ? "" : "s")
			+ " and ";
	out += std::to_string(t) + " second" + (t == 1 ? "" : "s");
	return out;
}
