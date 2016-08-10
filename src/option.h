/*
 * option.h: command option parsing functions
 */

#ifndef _OPTION_H
#define _OPTION_H

#ifndef EOF
# define EOF (-1)
#endif

#define NO_ARG  1
#define REQ_ARG 2

#ifdef __cplusplus
extern "C" {
#endif

/* index in argv being parsed */
extern int l_optind;

/* argument provided to option */
extern char *l_optarg;

/* struct describing an option for the getopt_long function */
struct l_option {
	const char *name;
	int type;
	int val;
};

/* opt_init: reset all getopt variables */
void opt_init();

/* l_getopt: parse command options */
int l_getopt(int argc, char **argv, const char *optstr);

/*
 * l_getopt_long: parse command options with suppport
 * for long options starting with '--'
 */
int l_getopt_long(int argc, char **argv, const char *optstr,
		const struct l_option *longopts);

/* l_opterr: return string describing getopt error */
char *l_opterr();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
