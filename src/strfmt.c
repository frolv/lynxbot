#include <string.h>
#include "strfmt.h"

/* fmtnum: format a number with commas */
void fmtnum(char *out, size_t size, const char *num)
{
	const char *s;
	size_t i, sz;

	if (size == 0)
		return;

	i = sz = 0;
	if (*num == '-') {
		*out++ = *num++;
		++sz;
	}
	while (num[i] && num[i] != '.')
		++i;
	s = num;
	while (*s && *s != '.' && sz < size - 1) {
		*out++ = *s++;
		++sz;
		if (--i && i % 3 == 0 && sz < size - 1) {
			*out++ = ',';
			++sz;
		}
	}
	if (*s == '.')
		strncat(out, s, size - sz);
	else
		*out = '\0';
}
