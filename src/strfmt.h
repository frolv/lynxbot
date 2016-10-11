/*
 * strfmt.h: functions to format strings
 */

#ifndef STRFMT_H
#define STRFMT_H

#define EFMT 1
#define EEOL 2

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A format struct defines a format sequence.
 * val is the format character.
 * fmtfun is a pointer to a function taking data as an argument and returning
 * a *dynamically allocated* string representing data. This is the string that
 * is inserted in place of the format sequence.
 */
struct format {
	int		val;
	const void	*data;
	char		*(*fmtfun)(const void *);
};

/* fmtnum: format a number with commas */
void fmtnum(char *out, size_t size, const char *num);

char *strfmt(char *out, size_t size, const char *str,
			const struct format *fmtchars);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* STRFMT_H */
