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
	/* gets the directory in which configuration files are located */
	std::string configdir();
	/* returns relative path of the config file within config dir */
	std::string config(const std::string &cfg);
	/* reads a json file and stores data in val */
	bool readJSON(const std::string &filename, Json::Value &val);
	/* writes the contents of val to a file */
	void writeJSON(const std::string &filename, Json::Value &val);
	/* parses bool from string into b */
	bool parseBool(bool &b, const std::string &s, std::string &err);
	/* parses int from string into i */
	bool parseInt(uint32_t &i, const std::string &s, std::string &err);
	/* conv_time: convert t to hours, minutes and seconds */
	std::string conv_time(time_t t);
	/* readnum: read number from string num into amt */
	bool readnum(char *num, int64_t *amt);
	/* upload: upload s to ptpb.pw */
	std::string upload(const std::string &s);
	/* decode: decode html encoded string */
	std::string decode(const std::string &s);
}
