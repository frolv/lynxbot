#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strfmt.h"

static const struct format *find_format(const struct format *fmt, int c);

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

char *strfmt(char *out, size_t size, const char *str,
			const struct format *fmtchars)
{
	char *start, *s, *in;
	size_t i, len;
	const struct format *f;

	start = out;
	s = start;
	i = 0;

	for (; *str && i < size - 1; ++str) {
		if (*str == '%') {
			if (!str[1])
				return NULL;

			if (str[1] == '%') {
				*s++ = *str++;
				continue;
			}

			if (!fmtchars || !(f = find_format(fmtchars, str[1])))
				return NULL;
			if (!(in = f->fmtfun(f->data)))
				return NULL;

			snprintf(s, size - i, "%s", in);
			len = strlen(in);
			i += len;
			s += len;

			/* skip format char */
			++str;

			free(in);
			continue;
		}
		*s++ = *str;
		++i;
	}
	*s = '\0';

	return start;
}

static const struct format *find_format(const struct format *fmt, int c)
{
	for (; fmt->val && fmt->data && fmt->fmtfun; ++fmt) {
		if (fmt->val == c)
			return fmt;
	}
	return NULL;
}
