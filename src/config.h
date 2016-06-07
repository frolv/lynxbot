#pragma once

#define STRING 0
#define LIST 1
#define OLIST 2

#include <string>
#include <unordered_map>
#include <vector>

struct setting {
	std::string key;
	int val_type;
};

class ConfigReader {
	public:
		ConfigReader(const std::string &path);
		bool read();
		bool write();
		std::string get(const std::string &key);
		void set(const std::string &key, const std::string &val);
	private:
		const std::string m_path;
		std::unordered_map<std::string, std::string> m_settings;
		std::unordered_map<std::string, std::vector<std::unordered_map<
			std::string, std::string>>> m_olist;
		std::string parseString(const std::string &buf);
		std::string parseList(const std::string &buf, std::string &err);
		std::string parseOList(const std::string &key,
				const std::string &buf, std::string &err);
		bool parseObj(const std::string &key, std::string &obj,
				std::string &err);
};
