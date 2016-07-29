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

/* index in argv being parsed */
extern int optind;

/* argument provided to option */
extern char *optarg;

/* struct describing an option for the getopt_long function */
struct option {
	const char *name;
	int type;
	int val;
};

/* opt_init: reset all getopt variables */
void opt_init();

/* getopt: parse command options */
int getopt(int argc, char **argv, const char *optstr);

/*
 * getopt_long: parse command options with suppport
 * for long options starting with '--'
 */
int getopt_long(int argc, char **argv, const char *optstr,
		const struct option *longopts);

/* opterr: return string describing getopt error */
char *opterr();

#endif
