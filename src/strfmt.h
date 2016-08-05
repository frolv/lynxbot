/*
 * strfmt.h: functions to format strings
 */

#ifndef _STRFMT_H
#define _STRFMT_H

#ifdef __cplusplus
extern "C" {
#endif

/* fmtnum: format a number with commas */
void fmtnum(char *out, size_t size, const char *num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
