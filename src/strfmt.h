/*
 * strfmt.h: functions to format strings
 */

#ifndef STRFMT_H
#define STRFMT_H

#define EFMT 1
#define EEOL 2
#define EINV 3

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

/*
 * strfmt:
 * Scan a string for percent format sequences with the format
 * characters given in fmtchars. Replace format sequences in the
 * string with the return value of fmtfun when called with data.
 * Write the resulting string to out, with at most size chars.
 *
 * Returns 0 on success, or one of the following errors:
 * EFMT - invalid format character found in str
 * EEOL - str ends with a '%'
 * EINV - could not use format function
 */
int strfmt(char *out, size_t size, const char *str,
		      const struct format *fmtchars);


/* strfmt format functions for common data types */
char *dupstr(const void *a);
char *plainnum(const void *a);
char *commanum(const void *a);
char *atnick(const void *a);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* STRFMT_H */
