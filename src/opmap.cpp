#include "opmap.h"

bool is_associative(const char &token, const uint8_t &type)
{
	const std::pair<uint8_t, uint8_t> pair = operators.find(token)->second;
	return pair.second == type;
}

int8_t compare_precedence(const char &t1, const char &t2)
{
	const std::pair<uint8_t, uint8_t> p1 = operators.find(t1)->second;
	const std::pair<uint8_t, uint8_t> p2 = operators.find(t2)->second;
	return p1.first - p2.first;
}
