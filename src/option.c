#include <string.h>
#include "lynxbot.h"
#include "option.h"

#define INV_OPT 0
#define ERR_SIZE 256

int l_optind;
char *l_optarg;

static int optopt;
static char *next;
static char error[ERR_SIZE];

static int parseopt(int argc, char **argv, const char *optstr, int c);
static int parselong(int argc, char **argv,
		const struct l_option *longopts, char *lopt);

/* opt_init: reset all getopt variables */
void opt_init()
{
	l_optind = 0;
	optopt = '\0';
	l_optarg = next = NULL;
	error[0] = '\0';
}

/* getopt: parse command options */
int l_getopt(int argc, char **argv, const char *optstr)
{
	l_optarg = NULL;
	error[0] = '\0';
	if (!next || !*next) {
		if (++l_optind == argc || *argv[l_optind] != '-'
				|| strcmp(argv[l_optind], "-") == 0)
			return EOF;
		if (strcmp(argv[l_optind], "--") == 0) {
			++l_optind;
			return EOF;
		}
		next = argv[l_optind] + 1;
	}

	optopt = *next++;
	return parseopt(argc, argv, optstr, optopt);
}

/*
 * getopt_long: parse command options with suppport
 * for long options starting with '--'
 */
int l_getopt_long(int argc, char **argv, const char *optstr,
		const struct l_option *longopts)
{
	l_optarg = NULL;
	error[0] = '\0';
	if (!next || !*next) {
		if (++l_optind == argc || *argv[l_optind] != '-'
				|| strcmp(argv[l_optind], "-") == 0)
			return EOF;
		if (strcmp(argv[l_optind], "--") == 0) {
			++l_optind;
			return EOF;
		}
		next = argv[l_optind] + 1;
	}

	/* long option */
	if (*next == '-')
		return parselong(argc, argv, longopts, ++next);

	optopt = *next++;
	return parseopt(argc, argv, optstr, optopt);
}

char *l_opterr()
{
	return error;
}

/* type: determine whether c is a valid option and if it requires an argument */
static int type(const char *optstr, int c)
{
	const char *s;

	if (!(s = strchr(optstr, c)))
		return INV_OPT;
	return *++s == ':' ? REQ_ARG : NO_ARG;
}

/* parseopt: process a single option */
static int parseopt(int argc, char **argv, const char *optstr, int c)
{
	switch (type(optstr, c)) {
	case NO_ARG:
		return c;
	case REQ_ARG:
		if (!*(l_optarg = next)) {
			if (++l_optind == argc) {
				_sprintf(error, ERR_SIZE, "%s: option requires "
						"an argument -- '%c'",
						argv[0], c);
				return '?';
			}
			l_optarg = argv[l_optind];
			next = NULL;
		}
		next = NULL;
		return c;
	default:
		_sprintf(error, ERR_SIZE, "%s: invalid option -- '%c'",
				argv[0], c);
		return '?';
	}
}

/* parselong: process a single long option */
static int parselong(int argc, char **argv,
		const struct l_option *longopts, char *lopt)
{
	if ((next = strchr(lopt, '=')))
		*next = '\0';

	for (; longopts->name && longopts->type && longopts->val; ++longopts) {
		if (strcmp(longopts->name, lopt) != 0)
			continue;
		if (longopts->type == NO_ARG) {
			if (next) {
				_sprintf(error, ERR_SIZE, "%s: option '--%s'"
						" doesn't allow an argument",
						argv[0], lopt);
				return '?';
			}
			return longopts->val;
		}
		if (!next) {
			if (++l_optind == argc) {
				_sprintf(error, ERR_SIZE, "%s: option '%s' "
						"requires an argument",
						argv[0], lopt);
				return '?';
			}
			l_optarg = argv[l_optind];
		} else {
			l_optarg = next + 1;
		}
		next = NULL;
		return longopts->val;
	}
	_sprintf(error, ERR_SIZE, "%s: unrecognized option '--%s'",
			argv[0], lopt);
	return '?';
}
