#include <iostream>
#include <sstream>
#include <fstream>
#ifdef __linux__
 #include <unistd.h>
#endif
#ifdef _WIN32
 #include <Windows.h>
#endif
#include "utils.h"

#define MAX_PATH_LEN 8192

bool utils::startsWith(const std::string &str, const std::string &prefix)
{
	return str.find(prefix) == 0;
}

bool utils::endsWith(const std::string &str, const std::string &suffix)
{
	return str.find(suffix) == str.length() - suffix.length();
}

std::vector<std::string> &utils::split(const std::string &str, char delim, std::vector<std::string> &elems)
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
static std::string appdir_win()
{
	char buf[MAX_PATH_LEN];
	int bytes;
	if ((bytes = GetModuleFileName(NULL, buf, sizeof(buf))) == 0) {
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
static std::string appdir_unix()
{
	char buf[MAX_PATH_LEN];
	if (!getcwd(buf, MAX_PATH_LEN)) {
		perror("getcwd");
		return "";
	}
	return std::string(buf);
}
#endif

std::string utils::appdir()
{
#ifdef __linux__
	return appdir_unix();
#endif
#ifdef _WIN32
	return appdir_win();
#endif
}

bool utils::readJSON(const std::string &filename, Json::Value &val)
{
	Json::Reader reader;
	std::ifstream fileStream(appdir() + "/json/" + filename, std::ifstream::binary);

	if (!reader.parse(fileStream, val)) {
		std::cerr << reader.getFormattedErrorMessages() << std::endl;
		return false;
	}
	return true;
}
