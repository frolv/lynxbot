#pragma once

#define STRING 0
#define LIST 1
#define OLIST 2

#include <string>
#include <unordered_map>

struct setting {
	std::string key;
	int val_type;
};

class ConfigReader {
	public:
		ConfigReader(const char *path);
		bool read();
		bool write();
		std::string getSetting(const std::string &setting);
	private:
		const char *m_path;
		std::unordered_map<std::string, std::string> m_settings;
		bool readSetting(FILE *f, int i, int *line);
		bool readString(FILE *f, int i, int *line);
		bool readList(FILE *f, int i, int *line);
		bool readOList(FILE *f, int i, int *line);
};
