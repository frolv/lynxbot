#ifndef _OPTION_H
#define _OPTION_H

#ifndef EOF
# define EOF (-1)
#endif

extern int optind;

struct option {
	const char *name;
	int type;
	int val;
};

void opt_init();
int getopt(int argc, char **argv, const char *optstr);
int getopt_long(int argc, char **argv, const char *optstr,
		const struct option *longopts);

#endif
