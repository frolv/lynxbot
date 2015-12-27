#include "OpMap.h"

bool isAssociative(const char &token, const uint8_t &type) {
	const std::pair<uint8_t, uint8_t> pair = opMap.find(token)->second;
	return pair.second == type;
}

int8_t comparePrecedence(const char &t1, const char &t2) {
	const std::pair<uint8_t, uint8_t> p1 = opMap.find(t1)->second;
	const std::pair<uint8_t, uint8_t> p2 = opMap.find(t2)->second;
	return p1.first - p2.first;
}