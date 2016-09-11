#pragma once

#define STRING 0
#define LIST 1
#define OLIST 2

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

struct setting {
	std::string key;	/* setting key */
	int val_type;		/* type of setting value */
	std::string comment;	/* comment written above setting */
	bool blank;		/* write a blank line after setting */
};

class ConfigReader {
	public:
		ConfigReader(const char *path);
		bool read();
		void write();
		std::string get(const std::string &key);
		void set(const std::string &key, const std::string &val);
		const char *path();
		std::unordered_map<std::string, std::vector<std::unordered_map<
			std::string, std::string>>> &olist();
	private:
		const char *config_path;
		std::unordered_map<std::string, std::string> setmap;
		std::unordered_map<std::string, std::vector<std::unordered_map<
			std::string, std::string>>> olistmap;
		std::string parse_string(const std::string &buf);
		std::string parse_list(const std::string &buf, std::string &err);
		std::string parse_olist(const std::string &key,
				const std::string &buf, std::string &err);
		bool parse_obj(const std::string &key, std::string &obj,
				std::string &err);
		void write_setting(std::ofstream &writer, size_t ind);
};
