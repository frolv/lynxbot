#include "Utils.h"

bool endsWith(const std::string &str, const std::string &ending) {

	if (str.length() < ending.length()) {
		return false;
	}

	// if string::compare returns 0 the two strings have the same character sequence
	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;

}