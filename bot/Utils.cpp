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