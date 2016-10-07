/*
 * strfmt.h: functions to format strings
 */

#ifndef STRFMT_H
#define STRFMT_H

#ifdef __cplusplus
extern "C" {
#endif

struct format {
	int	val;
	void	*data;
	char	(*fmtfun)(void *);
};

/* fmtnum: format a number with commas */
void fmtnum(char *out, size_t size, const char *num);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* STRFMT_H */
