#pragma once

#include <unordered_map>

#define LEFT_ASSOC 0
#define RIGHT_ASSOC 1

typedef std::unordered_map<char, std::pair<uint8_t, uint8_t>> opmap;

const opmap::value_type assocs[] = {
	opmap::value_type('+', std::make_pair<uint8_t, uint8_t>(1, LEFT_ASSOC)),
	opmap::value_type('-', std::make_pair<uint8_t, uint8_t>(1, LEFT_ASSOC)),
	opmap::value_type('*', std::make_pair<uint8_t, uint8_t>(2, LEFT_ASSOC)),
	opmap::value_type('/', std::make_pair<uint8_t, uint8_t>(2, LEFT_ASSOC)),
	opmap::value_type('^', std::make_pair<uint8_t, uint8_t>(3, RIGHT_ASSOC)),
	opmap::value_type('p', std::make_pair<uint8_t, uint8_t>(3, RIGHT_ASSOC)),
	opmap::value_type('m', std::make_pair<uint8_t, uint8_t>(3, RIGHT_ASSOC))
};

const opmap opMap(assocs, assocs + sizeof(assocs) / sizeof(assocs[0]));

/* Check whether the token has the specified associativity type. */
bool isAssociative(const char &token, const uint8_t &type);
/* Compares the precedence of two operators.
   Returns +ve, 0 or -ve number if t1 has higher, equal or lower precedence than t2, respectively. */
int8_t comparePrecedence(const char &t1, const char &t2);