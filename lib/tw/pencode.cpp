#include <ctype.h>
#include <sstream>
#include <tw/pencode.h>

bool valid(char c);

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

bool valid(char c)
{
	return isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}
