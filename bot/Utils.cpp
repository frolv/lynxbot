#include "stdafx.h"

bool startsWith(const std::string &str, const std::string &prefix) {
	return str.find(prefix) == 0;
}

std::vector<std::string> &split(const std::string &str, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}
	return elems;
}

std::vector<std::string> readSettings() {

	// get application directory
	char pBuf[1000];
	int bytes = GetModuleFileName(NULL, pBuf, sizeof(pBuf));
	if (bytes == 0) {
		throw std::runtime_error("Could not get current directory.");
	}

	// strip filename from path
	std::string path(pBuf);
	path = path.substr(0, path.find_last_of("\\"));

	// open settings.cfg
	std::ifstream cfgreader(path + "\\settings.txt");
	if (!cfgreader.is_open()) {
		throw std::runtime_error("Could not locate settings.txt");
	}

	std::string line;
	std::vector<std::string> info;

	while (std::getline(cfgreader, line)) {

		// remove whitespace
		line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
		 
		// lines starting with * are comments
		if (startsWith(line, "*")) {
			continue;
		}
		else {
			// line format is key:value
			if (line.find(":") == std::string::npos) {
				throw std::runtime_error("Invalid settings file.");
			}
			info.push_back(line.substr(line.find(":") + 1));
		}

		if (info.size() == 3) {
			break;
		}

	}

	cfgreader.close();
	return info;

}