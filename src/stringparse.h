/*
 * stringparse.h: functions to manipulate and parse data from C strings
 */

#ifndef _STRINGPARSE_H
#define _STRINGPARSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* shift_l: move every char in s one to the left */
void shift_l(char *s);

/* parsenum: read number from s into num */
int parsenum(const char *s, int64_t *res);

/*
 * parsenum_mult: read number from s into num, supporting numbers ending
 * in k, m or b representing multiples of 1e3, 1e6 and 1e9, respectively
 */
int parsenum_mult(const char *s, int64_t *amt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
