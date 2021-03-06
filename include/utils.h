#pragma once

#include <vector>
#include <json/json.h>

namespace utils {

	/* splits a string into separate tokens using specified delimiter */
	std::vector<std::string> &split(const std::string &str, char delim,
		std::vector<std::string> &elems);
	/* formats an integer string with commas */
	std::string format_int(std::string integer);
	/* gets the directory in which configuration files are located */
	std::string configdir();
	/* returns relative path of the config file within config dir */
	std::string config(const std::string &cfg);
	/* reads a json file and stores data in val */
	bool read_json(const std::string &filename, Json::Value &val);
	/* writes the contents of val to a file */
	void write_json(const std::string &filename, Json::Value &val);
	/* parses bool from string into b */
	bool parse_bool(bool &b, const std::string &s, std::string &err);
	/* parses int from string into i */
	bool parse_int(uint32_t &i, const std::string &s, std::string &err);
	/* conv_time: convert t to hours, minutes and seconds */
	std::string conv_time(time_t t);
	/* upload: upload s to ptpb.pw */
	std::string upload(const std::string &s);
	/* decode: decode html encoded string */
	std::string decode(const std::string &s);
	/* parse_time: extract time and date from ftime */
	std::string parse_time(const std::string &ftime, bool since);
}
