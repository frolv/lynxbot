#include <string.h>
#include "stringparse.h"

/* shift_l: move every char in s one to the left */
void shift_l(char *s)
{
	while ((s[0] = s[1]))
		++s;
}

/* parsenum: read number from s into num */
int parsenum(const char *s, int64_t *num)
{
	int neg;
	int64_t n;

	n = neg = 0;
	if (*s == '-') {
		neg = 1;
		++s;
	}

	for (; *s; ++s) {
		if (*s < '0' || *s > '9')
			return 0;
		n = 10 * n + (*s - '0');
	}

	*num = neg ? -n : n;
	return 1;
}

/*
 * parsenum_mult: read number from s into num, supporting numbers ending
 * in k, m or b representing multiples of 1e3, 1e6 and 1e9, respectively
 */
int parsenum_mult(const char *s, int64_t *num)
{
	const char *last;
	int64_t n, mult;

	last = s + strlen(s) - 1;
	mult = 1;

	if (*s == '-') {
		mult = -1;
		++s;
	}

	if (*last == 'k' || *last == 'm' || *last == 'b') {
		switch (*last) {
		case 'k':
			mult *= 1000;
			break;
		case 'm':
			mult *= 1000000;
			break;
		case 'b':
			mult *= 1000000000;
			break;
		}
	}

	n = 0;
	for (; s < last; ++s) {
		if (*s < '0' || *s > '9')
			return 0;
		n = 10 * n + (*s - '0');
	}

	*num = n * mult;
	return 1;
}
