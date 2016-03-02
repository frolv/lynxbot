#pragma once

#include <vector>
#include <json/json.h>
#include <WinSock2.h>

namespace utils {

	/* Returns true if str starts with prefix. */
	bool startsWith(const std::string &str, const std::string &prefix);
	/* Returns true if str ends with suffix. */
	bool endsWith(const std::string &str, const std::string &suffix);
	/* Splits a string into separate tokens using specified delimiter. */
	std::vector<std::string> &split(const std::string &str, char delim, std::vector<std::string> &elems);
	/* Formats an integer string with commas. */
	std::string formatInteger(std::string &integer);
	/* Gets the directory in which the application is located. */
	std::string getApplicationDirectory();
	/* Reads a .json file and stores data in val. */
	bool readJSON(const std::string &filename, Json::Value &val);
	/* Opens and connects to a socket. */
	bool socketConnect(SOCKET &s, WSADATA &w, const char *port, const char *server);
}