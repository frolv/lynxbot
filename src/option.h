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

extern int optind;
extern char *optarg;

struct option {
	const char *name;
	int type;
	int val;
};

void opt_init();
int getopt(int argc, char **argv, const char *optstr);
int getopt_long(int argc, char **argv, const char *optstr,
		const struct option *longopts);
char *opterr();

#endif
