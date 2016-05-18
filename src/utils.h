#pragma once

#include <vector>
#include <json/json.h>

namespace utils {

	/* returns true if str starts with prefix */
	bool startsWith(const std::string &str, const std::string &prefix);
	/* returns true if str ends with suffix */
	bool endsWith(const std::string &str, const std::string &suffix);
	/* splits a string into separate tokens using specified delimiter */
	std::vector<std::string> &split(const std::string &str, char delim,
		std::vector<std::string> &elems);
	/* formats an integer string with commas */
	std::string formatInteger(std::string integer);
	/* gets the directory in which the application is located */
	std::string appdir();
	/* reads a .json file and stores data in val */
	bool readJSON(const std::string &filename, Json::Value &val);
}
