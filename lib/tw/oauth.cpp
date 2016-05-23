#include <ctype.h>
#include <random>
#include <sstream>
#include <tw/base64.h>
#include <tw/oauth.h>

#define NBYTES 32

static bool valid(char c);

std::string tw::pencode(const std::string &s)
{
	std::ostringstream enc;
	for (unsigned char c : s) {
		if (valid(c)) {
			enc << c;
		} else {
			enc << '%';
			enc << std::hex << std::uppercase << (int)c;
		}
	}
	return enc.str();
}

std::string tw::noncegen()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> d(1, 255);
	char s[NBYTES];
	std::string enc, out;

	/* generate 32 pseudorandom bytes */
	for (int i = 0; i < NBYTES; ++i)
		s[i] = d(gen);

	/* base64 encode the bytes and remove non alphanumeric chars */
	enc = base64_enc(s, NBYTES);
	for (char c : enc) {
		if (isalnum(c))
			out += c;
	}
	return out;
}

static bool valid(char c)
{
	return isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}
