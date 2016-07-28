#include <string.h>
#include "lynxbot.h"
#include "option.h"

#define INV_OPT 0

#define ERR_SIZE 256

int optind;
char *optarg;

static int optopt;
static char *next;
static char error[ERR_SIZE];

static int type(const char *optstr, int c);

/* opt_init: reset all getopt variables */
void opt_init()
{
	optind = 0;
	optopt = '\0';
	optarg = next = NULL;
	error[0] = '\0';
}

/* getopt: parse command options */
int getopt(int argc, char **argv, const char *optstr)
{
	optarg = NULL;
	error[0] = '\0';
	if (!next || !*next) {
		if (++optind == argc || *argv[optind] != '-'
				|| strcmp(argv[optind], "-") == 0)
			return EOF;
		if (strcmp(argv[optind], "--") == 0) {
			++optind;
			return EOF;
		}
		next = argv[optind] + 1;
	}

	optopt = *next++;
	switch (type(optstr, optopt)) {
	case NO_ARG:
		return optopt;
	case REQ_ARG:
		if (!*(optarg = next)) {
			if (++optind == argc) {
				_sprintf(error, ERR_SIZE, "%s: option requires "
						"an argument -- '%c'",
						argv[0], optopt);
				return '?';
			}
			optarg = argv[optind];
			next = NULL;
		}
		next = NULL;
		return optopt;
	default:
		_sprintf(error, ERR_SIZE, "%s: invalid option -- '%c'",
				argv[0], optopt);
		return '?';
	}
}

char *opterr()
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
