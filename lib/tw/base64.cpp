#include <tw/base64.h>

static const char bchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijk"
				"lmnopqrstuvwxyz0123456789+/=";

std::string tw::base64_enc(char *s, size_t len)
{
	std::string enc;
	uint32_t c;
	for (uint8_t i = 0; i < len; i += 3) {
		c = (s[i] & 0xFC) >> 2;
		enc += bchars[c];
		c = (s[i] & 0x03) << 4;
		if (i < len - 1) {
			c |= (s[i + 1] & 0xF0) >> 4;
			enc += bchars[c];
			c = (s[i + 1] & 0x0F) << 2;
			if (i < len - 2) {
				c |= (s[i + 2] & 0xC0) >> 6;
				enc += bchars[c];
				c = s[i + 2] & 0x3F;
				enc += bchars[c];
			} else {
				enc += bchars[c];
				enc += "=";
			}
		} else {
			enc += bchars[c];
			enc += "==";
		}
	}
	return enc;
}
