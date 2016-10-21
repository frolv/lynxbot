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

/*
 * strfmt:
 * scan a string for percent format sequences with the format
 * characters given replace fmtchars. Replace format sequences with
 * the return value of fmtfun replace the format struct. Write
 * resulting string to out, with at most size chars.
 *
 * Returns 0 on success, or one of the following errors:
 * EFMT - invalid format character found in str
 * EEOL - str ends with a '%'
 * EINV - could not use format function
 */
int strfmt(char *out, size_t size, const char *str,
		      const struct format *fmtchars)
{
	char *s, *replace;
	size_t i, len;
	const struct format *f;

	s = out;
	i = 0;

	for (; *str && i < size - 1; ++str) {
		if (*str == '%') {
			if (!str[1])
				return EEOL;

			if (str[1] == '%') {
				*s++ = *str++;
				++i;
				continue;
			}

			if (!fmtchars || !(f = find_format(fmtchars, str[1])))
				return EFMT;
			if (!(replace = f->fmtfun(f->data)))
				return EINV;

			snprintf(s, size - i, "%s", replace);
			len = strlen(replace);
			i += len;
			s += len;

			/* skip format char */
			++str;

			free(replace);
			continue;
		}
		*s++ = *str;
		++i;
	}
	/*
	 * This difference is greater than size if the length of the string
	 * replace exceeds the amount of remaining space, at which point
	 * snprintf truncates replace and null terminates the result.
	 */
	if (i < size)
		*s = '\0';

	return 0;
}

char *dupstr(const void *a)
{
	return strdup((const char *)a);
}

char *plainnum(const void *a)
{
	size_t len;
	int i;
	char *num;

	i = *(int *)a;
	len = 1;
	while ((i /= 10))
		++len;

	if (!(num = malloc(len + 1)))
		return NULL;
	sprintf(num, "%d", *(int *)a);
	return num;
}

char *commanum(const void *a)
{
	size_t len;
	int i;
	char *num;
	char buf[128];

	i = *(int *)a;
	len = 1;
	while ((i /= 10))
		++len;

	/* comma after every 3 digits */
	len += (len - 1) / 3;

	if (!(num = malloc(len + 1)))
		return NULL;
	snprintf(buf, 128, "%d", *(int *)a);
	fmtnum(num, len + 1, buf);

	return num;
}

char *atnick(const void *a)
{
	size_t len;
	char *out;

	len = strlen((const char *)a) + 2;
	if (!(out = malloc(len + 1)))
		return NULL;
	sprintf(out, "@%s,", (const char *)a);

	return out;
}

static const struct format *find_format(const struct format *fmt, int c)
{
	for (; fmt->val && fmt->data && fmt->fmtfun; ++fmt) {
		if (fmt->val == c)
			return fmt;
	}
	return NULL;
}
